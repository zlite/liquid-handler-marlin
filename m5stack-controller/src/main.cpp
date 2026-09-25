#include <M5Stack.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "ch340_host.h"
#include "teaching.h"
#include "dry_run.h"
#include "web_page.h"
#if __has_include("network_secrets.h")
#include "network_secrets.h"
#else
constexpr char WIFI_SSID[]="";
constexpr char WIFI_PASSWORD[]="";
#endif

USB usb;
Ch340Host ender(usb);
WebServer web(80);
Preferences preferences;
constexpr char HOSTNAME[]="dispenser";
constexpr char A_CALIBRATION[]="M92 A1600";
const char *pointNames[]={"a1","a12","h1","h12","reservoir","dispense","travel"};
Position points[7],position,candidate;
bool present[7]={},havePosition=false,ready=false,connected=false,xyHomed=false,zHomed=false;
bool busy=false,awaiting=false,positionSeen=false,identitySeen=false,saved=false,mdns=false;
String calName="96-well plate",notes,message="Starting USB",logText,job;
String commands[24];unsigned commandCount=0,commandIndex=0,lineNumber=0,capabilities=0;
int captureIndex=-1;
uint32_t mountedAt=0,sentAt=0,lastRX=0,lastDraw=0,lastWeb=0;
char inputLine[512];size_t inputLength=0;
bool droppingLine=false,hostOK=false;
bool aLimitKnown=false,aLimitTriggered=false;
int pendingALimit=-1;
uint32_t aLimitAt=0,lastLimitPoll=0,aLimitSamples=0;
constexpr uint32_t LIMIT_POLL_MS=200,LIMIT_STALE_MS=1000;
const char *LIMIT_MONITOR="Monitor A switch";
const char *HOLD_TEST="Stationary motor test";
const char *MOTION_TEST="Moving motor test";
uint32_t holdStartedAt=0,holdLastPoll=0,holdSamples=0,holdTriggers=0;
bool samplingCommand(){return (job==HOLD_TEST&&commandIndex==4)||(job==MOTION_TEST&&commandIndex==9);}
String aHomingTrace;
bool dryRunning=false,dryStop=false;
unsigned dryVisit=0;

bool aLimitFresh(){return connected&&ready&&aLimitKnown&&millis()-aLimitAt<LIMIT_STALE_MS;}

void logMessage(const String &s){
  Serial.println(s);
  logText+=s+"\n";
  if(logText.length()>3000)logText.remove(0,logText.length()-3000);
  if(busy&&(job=="Diagnose A homing"||job=="Read motion settings"||job=="Set A calibration"||job=="Move A down 5 mm"||job==HOLD_TEST||job==MOTION_TEST)){
    aHomingTrace+=s+"\n";
    if(aHomingTrace.length()>12000)aHomingTrace.remove(0,aHomingTrace.length()-12000);
  }
}
void serializePosition(JsonObject o,const Position &p){o["x"]=p.x;o["y"]=p.y;o["z"]=p.z;o["a"]=p.a;}
bool geometryOK(){return present[0]&&present[1]&&present[2]&&validPlate(points[0],points[1],points[2]);}
float checkError(){return distanceXY(wellPosition(points[0],points[1],points[2],7,11),points[3]);}
bool plateVerified(){return geometryOK()&&present[3]&&checkError()<=1.0f;}
bool heightsOK(){return present[4]&&present[5]&&present[6]&&points[6].z>points[4].z&&points[6].z>points[5].z;}
void calibration(JsonDocument &d){
  d["schema"]=1;d["name"]=calName;d["notes"]=notes;d["rows"]=8;d["columns"]=12;d["nominalPitchMm"]=9;
  d["units"]="mm";d["plateVerified"]=plateVerified();d["heightsValid"]=heightsOK();
  auto ps=d["points"].to<JsonObject>();
  for(unsigned i=0;i<7;i++)if(present[i])serializePosition(ps[pointNames[i]].to<JsonObject>(),points[i]);
  if(plateVerified()&&heightsOK()){
    auto wells=d["wells"].to<JsonObject>();
    for(unsigned r=0;r<8;r++)for(unsigned c=0;c<12;c++){
      Position p=wellPosition(points[0],points[1],points[2],r,c);p.z=points[5].z;
      String name=String(char('A'+r))+String(c+1);
      auto w=wells[name].to<JsonObject>();w["x"]=p.x;w["y"]=p.y;w["z"]=p.z;
    }
  }
}
bool persist(){
  JsonDocument d;calibration(d);d.remove("wells");String s;serializeJson(d,s);
  saved=preferences.putString("calibration",s)==s.length();return saved;
}
void load(){
  String s=preferences.getString("calibration","");if(s.isEmpty())return;
  JsonDocument d;if(deserializeJson(d,s)||d["schema"].as<int>()!=1)return;
  calName=d["name"]|"96-well plate";notes=d["notes"]|"";
  for(unsigned i=0;i<7;i++){
    JsonObject p=d["points"][pointNames[i]].as<JsonObject>();
    if(!p["x"].is<float>()||!p["y"].is<float>()||!p["z"].is<float>()||!p["a"].is<float>())continue;
    Position v{p["x"].as<float>(),p["y"].as<float>(),p["z"].as<float>(),p["a"].as<float>()};
    if(inside(v)&&std::isfinite(v.a)){points[i]=v;present[i]=true;}
  }
  saved=true;
}
void fail(const String &reason){
  if(message.startsWith("Connection fault:"))return;
  dryRunning=false;
  message="Connection fault: "+reason+". Reconnect USB to recheck.";
  logMessage(message);busy=awaiting=ready=false;xyHomed=zHomed=false;havePosition=false;
  aLimitKnown=false;
  // Best effort ordered release, never replay a timed-out movement.
  if(connected){ender.write((job=="Move A down 5 mm"||job==MOTION_TEST)?"M400\nG90\nM211 S1\nM84\n":"M400\nM84\n");}
}
void beginJob(const String &name,std::initializer_list<String> sequence,int capture=-1){
  job=name;captureIndex=capture;commandCount=0;commandIndex=0;busy=true;awaiting=false;
  for(const auto &s:sequence)commands[commandCount++]=s;
  if(name!=LIMIT_MONITOR){message=name;logMessage("Operation: "+name);}
}
void nextDryVisit(){
  Position p=dryRunTarget(points,dryVisit);
  String label=dryVisit%9?String(char('A'+dryVisit%9-1))+String(dryVisit/9+1):"Reservoir";
  String name="Dry run "+String(dryVisit/9+1)+"/12: "+label;
  // Lift only across the reservoir/plate boundary. Adjacent wells share Z.
  if(dryRunNeedsClearance(dryVisit)){
    beginJob(name,{"G21","G90","M211 S1",
      "G1 Z"+String(points[6].z,3)+" F300","M400",
      "G1 X"+String(p.x,3)+" Y"+String(p.y,3)+" F30000","M400",
      "G1 Z"+String(p.z,3)+" F300","M400","M114"});
  }else{
    beginJob(name,{"G21","G90",
      "G1 X"+String(p.x,3)+" Y"+String(p.y,3)+" F30000","M400","M114"});
  }
}
void startCheck(){
  lineNumber=0;identitySeen=false;capabilities=0;
  beginJob("Checking Marlin replies",{"M110 N0","M115","M114","M114","M114","M114","M114","M114","M114","M114","M114","M114","M400","M84"});
}
void finishJob(){
  if(dryRunning){
    if(!dryStop&&++dryVisit<DRY_VISITS){nextDryVisit();return;}
    dryRunning=false;
    beginJob(dryStop?"Dry run stopped at travel Z":"Dry run 96 wells finished",{"G90","G1 Z"+String(points[6].z,3)+" F300","M400","M114","M400","M84"});
    return;
  }
  if(job==LIMIT_MONITOR){busy=false;return;}
  if(job==HOLD_TEST||job==MOTION_TEST)logMessage(job+": "+String(holdSamples)+" readings, "+String(holdTriggers)+" triggered; motor release acknowledged");
  if(job=="Checking Marlin replies")ready=true;
  if(job=="Home X/Y"||job=="Home all")xyHomed=true;
  if(job=="Home Z"||job=="Home all")zHomed=true;
  if(captureIndex>=0){
    Position old=points[captureIndex];bool had=present[captureIndex],hadCheck=present[3];
    points[captureIndex]=candidate;present[captureIndex]=true;
    if(captureIndex<3)present[3]=false; // Recheck H12 after changing any grid anchor.
    if(!persist()){
      points[captureIndex]=old;present[captureIndex]=had;present[3]=hadCheck;
      message="Storage failed; capture was not saved";busy=false;return;
    }
  }
  message=ready?job+" complete · motors released":"Waiting for connection";
  if(captureIndex>=0)message="Saved "+String(pointNames[captureIndex])+" · motors released";
  busy=false;logMessage(message);
}
void receiveLine(const char *line){
  if(!*line)return;
  logMessage(String("RX ")+line);
  if(strncmp(line,"Error:",6)==0||strncmp(line,"Resend:",7)==0||strncmp(line,"!!",2)==0){fail(line);return;}
  if(!awaiting)return;
  if(commands[commandIndex]=="M115"){
    if(strstr(line,"FIRMWARE_NAME:Marlin")&&strstr(line,"Liquid Handler"))identitySeen=true;
    if(strncmp(line,"Cap:",4)==0)capabilities++;
  }
  if(commands[commandIndex]=="M114" && strncmp(line,"X:",2)==0){
    Position p;if(!parsePosition(line,p)||!inside(p)){fail("Invalid position reply");return;}
    position=p;havePosition=true;positionSeen=true;candidate=p;
  }
  if(commands[commandIndex]=="M119"){
    if(strcmp(line,"a_min: open")==0)pendingALimit=0;
    else if(strcmp(line,"a_min: TRIGGERED")==0)pendingALimit=1;
  }
  if(strcmp(line,"ok")==0||strncmp(line,"ok ",3)==0){
    if(commands[commandIndex]=="M119"){
      if(pendingALimit<0){fail("Missing A limit-switch reply");return;}
      aLimitTriggered=pendingALimit==1;aLimitKnown=true;aLimitAt=millis();aLimitSamples++;
      if(samplingCommand()){
        holdSamples++;if(aLimitTriggered)holdTriggers++;
        logMessage(String(job==HOLD_TEST?"HOLD_SAMPLE":"MOTION_SAMPLE")+" ms="+String(millis()-holdStartedAt)+" A="+(aLimitTriggered?"TRIGGERED":"open"));
      }
    }
    if(commands[commandIndex]=="M114"&&!positionSeen){fail("Missing complete position reply");return;}
    if(commands[commandIndex]=="M115"&&(!identitySeen||capabilities<10)){fail("Incomplete firmware identification");return;}
    if(job==HOLD_TEST&&commands[commandIndex]=="M17")holdStartedAt=millis();
    if(job==MOTION_TEST&&commandIndex==8)holdStartedAt=millis();
    awaiting=false;
    // Keep this one read-only command active for ten seconds after M17 / G1.
    // The queue remains busy; no other jobs or motor releases can interleave.
    if(!(samplingCommand()&&millis()-holdStartedAt<10000))commandIndex++;
    if(commandIndex==commandCount)finishJob();
  }
}
void serviceUSB(){
  usb.Task();
  if(ender.connected()!=connected){
    connected=ender.connected();mountedAt=millis();inputLength=0;droppingLine=false;
    ready=busy=awaiting=havePosition=xyHomed=zHomed=false;dryRunning=false;
    aLimitKnown=false;
    message=connected?"CH340 detected · waiting for Marlin":"Ender disconnected";
    logMessage(message);
  }
  if(!connected)return;
  for(unsigned packet=0;packet<16;packet++){
    uint8_t data[64];uint16_t n=sizeof(data);uint8_t rc=ender.read(data,n);
    if(rc==hrNAK)break;
    if(rc){fail("USB receive error "+String(rc));return;}
    if(!n)break;lastRX=millis();
    for(unsigned i=0;i<n;i++){
      char c=data[i];if(c=='\r')continue;
      if(c=='\n'){
        if(droppingLine){droppingLine=false;inputLength=0;fail("Overlong reply");return;}
        inputLine[inputLength]=0;receiveLine(inputLine);inputLength=0;
      }else if(inputLength<sizeof(inputLine)-1)inputLine[inputLength++]=c;
      else droppingLine=true;
    }
  }
  if(!busy&&!ready&&message=="CH340 detected · waiting for Marlin"&&millis()-mountedAt>2500&&millis()-lastRX>200)startCheck();
  if(awaiting){
    uint32_t limit=commands[commandIndex].startsWith("G28")?180000:(dryRunning||job.startsWith("Dry run")||job.startsWith("Home "))?60000:10000;
    if(millis()-sentAt>limit)fail("No acknowledgement for "+commands[commandIndex]);
  }else if(busy&&millis()-lastRX>15){
    if(samplingCommand()){
      if(millis()-holdLastPoll<LIMIT_POLL_MS)return;
      holdLastPoll=millis();
    }
    String body="N"+String(lineNumber)+" "+commands[commandIndex];uint8_t checksum=0;
    for(unsigned i=0;i<body.length();i++)checksum^=body[i];
    String wire=body+"*"+String(checksum)+"\n";
    logMessage("TX "+commands[commandIndex]);
    if(ender.write(wire.c_str())){fail("USB send failed");return;}
    lineNumber++;positionSeen=false;pendingALimit=-1;awaiting=true;sentAt=millis();
  }
}
void error(int code,const String &why){JsonDocument d;d["error"]=why;String s;serializeJson(d,s);web.send(code,"application/json",s);}
bool readBody(JsonDocument &d){if(web.arg("plain").length()>1024||deserializeJson(d,web.arg("plain"))){error(400,"Invalid request");return false;}return true;}
void stateResponse(){
  JsonDocument d;calibration(d);d.remove("wells");d["ready"]=ready;d["busy"]=busy;d["message"]=message;
  d["dryRunning"]=dryRunning;d["dryVisit"]=dryVisit;d["dryStopRequested"]=dryStop;
  d["xyHomed"]=xyHomed;d["zHomed"]=zHomed;d["saved"]=saved;d["log"]=logText;
  d["aLimit"]=aLimitFresh()?(aLimitTriggered?"TRIGGERED":"open"):"unknown";
  d["aLimitFresh"]=aLimitFresh();d["aLimitSamples"]=aLimitSamples;
  if(aLimitKnown)d["aLimitAgeMs"]=millis()-aLimitAt;
  if(havePosition)serializePosition(d["position"].to<JsonObject>(),position);
  if(!present[0]||!present[1]||!present[2])d["geometry"]="Capture A1, A12, and H1.";
  else if(!geometryOK())d["geometry"]="Check the taught points: expected 99 mm across, 63 mm down, and square axes.";
  else if(!present[3])d["geometry"]="Grid calculated. Capture H12 independently to verify it.";
  else d["geometry"]="H12 check error: "+String(checkError(),2)+" mm. "+(plateVerified()?"Plate verified.":"Above 1 mm; check and recapture the points.");
  d["heights"]=heightsOK()?"Heights saved. Confirm the travel path clears all obstacles.":"Capture reservoir, dispense, and travel heights. Travel Z must be higher than the working heights.";
  String s;serializeJson(d,s);web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",s);
}
void action(){
  JsonDocument d;if(!readBody(d))return;
  String requested=d["action"]|"";
  if(requested=="stop_dry_run"&&dryRunning){dryStop=true;web.send(202,"application/json","{\"accepted\":true}");return;}
  if(busy){error(409,"Wait for the current operation to finish");return;}
  if(!ready){error(409,"Marlin connection has not passed its checks");return;}
  String a=d["action"]|"";
  if(a=="dry_run"){
    if(!xyHomed||!zHomed||!havePosition){error(409,"Home X/Y and Z before the dry run");return;}
    for(bool p:present)if(!p){error(409,"Capture all seven calibration points first");return;}
    if(!dryRunValid(points,d["allowTwoMm"].as<bool>())){error(409,"Calibration failed dry-run checks; trial override allows H12 error up to 2 mm only");return;}
    dryVisit=0;dryStop=false;dryRunning=true;nextDryVisit();
  }
  else if(a=="invalidate"){xyHomed=zHomed=false;havePosition=false;message="Re-home after unmeasured movement";}
  else if(a=="home_all"||a=="home_xy"||a=="home_z"){
    if(!present[6]||!std::isfinite(points[6].z)||points[6].z<=0||points[6].z>250||!havePosition){
      error(409,"A saved travel height and current position report are required before homing");return;
    }
    const bool referenced=zHomed;
    const float lift=referenced?fmaxf(position.z,points[6].z):points[6].z;
    if(!referenced&&position.z+lift>250){error(409,"Clearance lift would exceed Z travel; establish Z from a clear position first");return;}
    String label=a=="home_all"?"Home all":a=="home_xy"?"Home X/Y":"Home Z";
    String home=a=="home_all"?"G28 R0":a=="home_xy"?"G28 X Y R0":"G28 Z R0";
    if(a!="home_z")xyHomed=false;
    if(a!="home_xy")zHomed=false;
    beginJob(label,{"G21","M211 S1",referenced?"G90":"G91",
      "G1 Z"+String(lift,3)+" F300","M400","G90",home,"M400","M114","M400","M84"});
  }
  else if(a=="position")beginJob("Read position",{"M400","M114","M84"});
  else if(a=="endstops")beginJob("Read limit switches",{"M400","M119","M84"});
  else if(a=="home_a_diag"){
    if(!aLimitFresh()||aLimitTriggered){error(409,"A switch must have a fresh OPEN reading before this test");return;}
    aHomingTrace="";
    beginJob("Diagnose A homing",{"M400","M119","M503","G21","G28 A R0","M400","M114","M119","M84"});
  }
  else if(a=="settings"){
    aHomingTrace="";
    beginJob("Read motion settings",{"M400","M503","M114","M119","M84"});
  }
  else if(a=="restore_a_steps"){
    aHomingTrace="";
    beginJob("Set A calibration",{"M400",A_CALIBRATION,"M503","M84"});
  }
  else if(a=="hold_a_test"){
    if(!aLimitFresh()||aLimitTriggered){error(409,"A switch must have a fresh OPEN reading before this test");return;}
    aHomingTrace="";holdStartedAt=holdLastPoll=holdSamples=holdTriggers=0;
    beginJob(HOLD_TEST,{"M400","M114","M119","M17","M119","M400","M114","M84"});
  }
  else if(a=="motion_a_test"){
    if(!aLimitFresh()||aLimitTriggered){error(409,"A switch must have a fresh OPEN reading before this test");return;}
    aHomingTrace="";holdStartedAt=holdLastPoll=holdSamples=holdTriggers=0;
    // Ordinary G1 acknowledges once queued, allowing M119 during stepping.
    // Keep the physical endstop active; restore coordinate mode / soft limits.
    beginJob(MOTION_TEST,{"M400",A_CALIBRATION,"M114","M119","M120","M211 S0","G21","G91","G1 A-5 F30","M119","M400","G90","M211 S1","M114","M119","M400","M84"});
  }
  else if(a=="move_a_down_5"){
    if(!aLimitFresh()||aLimitTriggered){error(409,"A switch must have a fresh OPEN reading before this test");return;}
    aHomingTrace="";
    // The reported A=0 from unsuccessful homing is not a physical reference.
    // Temporarily bypass software coordinates, retaining the physical endstop.
    beginJob("Move A down 5 mm",{"M400",A_CALIBRATION,"M503","M119","M120","M211 S0","G21","G91","G1 A-5 F60","M400","G90","M211 S1","M114","M119","M400","M84"});
  }
  else if(a=="capture"||a=="jog"){
    if(!xyHomed||!zHomed||!havePosition){error(409,"Home X/Y and Z before jogging or teaching");return;}
    if(a=="capture"){
      String p=d["point"]|"";int index=-1;for(unsigned i=0;i<7;i++)if(p==pointNames[i])index=i;
      if(index<0){error(400,"Unknown calibration point");return;}
      beginJob("Capture "+p,{"M400","M114","M84"},index);
    }else{
      String axis=d["axis"]|"";float delta=d["delta"]|NAN;
      if((axis!="X"&&axis!="Y"&&axis!="Z")||!std::isfinite(delta)||!(fabs(delta)==10||fabs(delta)==1||fabs(delta-0.1f)<1e-6||fabs(delta+0.1f)<1e-6)){error(400,"Invalid jog");return;}
      Position p=position;float *value=axis=="X"?&p.x:axis=="Y"?&p.y:&p.z;*value+=delta;
      if(!inside(p)){error(400,"Jog exceeds the configured travel limits");return;}
      String move="G1 "+axis+String(*value,3)+" F"+(axis=="Z"?"120":"600");
      beginJob("Jog "+axis,{"G21","G90",move,"M400","M114","M84"});
    }
  }else{error(400,"Unknown action");return;}
  web.send(202,"application/json","{\"accepted\":true}");
}
void setupWeb(){
  web.on("/",HTTP_GET,[]{web.send_P(200,"text/html",WEB_PAGE);});
  web.on("/api/state",HTTP_GET,stateResponse);web.on("/api/action",HTTP_POST,action);
  web.on("/api/a-homing-trace",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"text/plain",aHomingTrace);});
  web.on("/api/calibration",HTTP_GET,[]{JsonDocument d;calibration(d);String s;serializeJsonPretty(d,s);web.sendHeader("Content-Disposition","attachment; filename=liquid-handler-calibration.json");web.send(200,"application/json",s);});
  web.on("/api/metadata",HTTP_POST,[]{
    if(busy){error(409,"Wait for the operation to finish");return;}
    JsonDocument d;if(!readBody(d))return;String n=d["name"]|"",t=d["notes"]|"";
    if(n.length()>40||t.length()>160){error(400,"Name or notes too long");return;}
    String oldName=calName,oldNotes=notes;calName=n;notes=t;
    if(!persist()){calName=oldName;notes=oldNotes;error(500,"Storage failed");return;}web.send(200,"application/json","{}");
  });
  web.on("/api/clear",HTTP_POST,[]{
    if(busy){error(409,"Wait for the operation to finish");return;}
    if(preferences.isKey("calibration")&&!preferences.remove("calibration")){error(500,"Storage failed");return;}
    memset(present,0,sizeof(present));saved=false;web.send(200,"application/json","{}");
  });
  web.begin();
}
void draw(){
  // Only paint changed content, keeping shared SPI available for USB.
  static String previous;
  const bool fresh=aLimitFresh();
  String state=fresh?(aLimitTriggered?"TRIGGERED":"OPEN"):"UNKNOWN";
  String address=WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():WiFi.softAPIP().toString();
  String homed="Home XY: "+String(xyHomed?"yes":"no")+"  Z: "+String(zHomed?"yes":"no");
  String status=message;status.replace(" · "," / ");
  String coords=havePosition?"X "+String(position.x,1)+" Y "+String(position.y,1)+" Z "+String(position.z,1):"Position unavailable";
  String display=state+address+homed+status+coords+String(ready);
  if(display==previous)return;
  previous=display;
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE,BLACK);M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(8,8);M5.Lcd.print("dispenser.local");
  M5.Lcd.setTextSize(1);M5.Lcd.setCursor(8,32);M5.Lcd.print(address);
  M5.Lcd.setTextSize(2);M5.Lcd.setTextColor(ready?GREEN:YELLOW,BLACK);
  M5.Lcd.setCursor(8,52);M5.Lcd.print(ready?"Teaching controller":"Connecting / fault");
  M5.Lcd.setTextColor(WHITE,BLACK);
  // Three lines fit a full task or connection-fault message.
  for(unsigned i=0;i<3;i++){
    M5.Lcd.setCursor(8,80+i*20);M5.Lcd.print(status.substring(i*25,(i+1)*25));
  }
  M5.Lcd.setTextSize(1);M5.Lcd.setCursor(8,151);M5.Lcd.print("Last reported: "+coords);
  M5.Lcd.setTextSize(2);M5.Lcd.setCursor(8,171);M5.Lcd.print(homed);
  M5.Lcd.setTextColor(fresh?(aLimitTriggered?RED:GREEN):YELLOW,BLACK);
  M5.Lcd.setCursor(8,207);M5.Lcd.print("A switch: "+state);
}

void setup(){
  Serial.setTxBufferSize(8192);Serial.begin(115200);
  pinMode(5,OUTPUT);digitalWrite(5,HIGH);pinMode(4,OUTPUT);digitalWrite(4,HIGH);
  M5.begin(true,false,false,false);M5.Lcd.setBrightness(200);
  M5.Lcd.fillScreen(BLACK);M5.Lcd.setTextWrap(false);
  preferences.begin("liquid-teach",false);load();
  WiFi.setHostname(HOSTNAME);WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  uint32_t start=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-start<25000)delay(100);
  if(WiFi.status()!=WL_CONNECTED){WiFi.mode(WIFI_AP_STA);WiFi.setAutoReconnect(true);WiFi.softAP("LiquidHandler-Teach","teachplate96");}
  mdns=MDNS.begin(HOSTNAME);if(mdns)MDNS.addService("http","tcp",80);
  setupWeb();hostOK=usb.Init()==0;message=hostOK?"Waiting for Ender USB":"USB host initialization failed";
  logMessage("Teaching UI: http://"+(WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():WiFi.softAPIP().toString()));draw();
}
void loop(){
  if(hostOK)serviceUSB();
  M5.update();
  // Redraw during long command waits too (e.g. homing), but outside reply bursts.
  if(millis()-lastDraw>50&&millis()-lastRX>30&&(!awaiting||millis()-sentAt>250)){
    lastDraw=millis();draw();
  }
  // Serve browser only outside reply bursts, avoiding USB starvation.
  if((!awaiting||millis()-sentAt>250) && millis()-lastRX>30){
    if(millis()-lastWeb>30){lastWeb=millis();web.handleClient();}
  }
  // Continuous read-only monitoring, serialized with all existing jobs.
  // M119 does not energize motors; never inject releases into panel-driven motion.
  if(ready&&!busy&&millis()-lastLimitPoll>=LIMIT_POLL_MS&&millis()-lastRX>40){
    lastLimitPoll=millis();beginJob(LIMIT_MONITOR,{"M119"});
  }
}
