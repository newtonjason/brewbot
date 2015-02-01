  #include <OneWire.h>//Need to have the OneWire library to use the Dallas DS18B20 sensors
  
//Design control strategy:
//Serial communication
//Input serial command in following format: XXX YYY ZZZ
//XXX stands for part of system: HLT, MSH, REC, KET, (Hot liquor tank, mash tun, recirculation, kettle)
//YYY stands for SUBSYSTEM to perform: TMP, IVL, OVL, ATS, MIX, POW (temperature sensors, inlet valve, outlet valve, automated temperature system, mixer, power level) 
//ZZZ STANDS FOR ACTION TO PERFORM: 7000, on, off, 100 (set temperature to 70.00 celcius, on, off, set power level to 100% on time)
  
//To Do actions:
//1. Find digital pin locations for mash, recirulation and kettle temperature sensors
//2. Write code for water pressure sensing.
//3. Write code 
  
//Arduino Pins used:
//Digital Pins
//Analog Pins
int intx; 
String cmd; //system variable cmd is where the command sent to the arduino is stored.
long hlttemplast = 0;    long kettemplast = 0;     long mshtemplast = 0; // variables meant to keep track of the last time the temperature of the hlt, mash tun, or ket was calculated
long timerl = 0;         long mashwaterlast = 0;   int ihlv = 0;// the time variable to keep track of the last time the refresh actions command was run.
long hltssron = 0;       long hltssroff = 0;       long mashwateradd = 0;//variables meant to keep track of how long the hlt element has been on or off
long ketssron = 0;       long ketssroff = 0;       int tempvol = 0;//variables meant to keep track of how long the ket element has been on or off
int lvlres;              long hltvollast = 0;      long hltvolsum = 0;        long hltvolcount = 0;

//I decided to store the location (pin location on arduino) for each component in my system in an array. This way if I wish to move somethign around later, I need only change this number here instead of searching for digitalwrite values in my code.
int componentlocation[] = {22,48,39,33,37,36,40,35,47,23,32,34,46,30,0 ,0 ,0 ,13,14,15,27 };
//POSITION                {0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10,11,12,13,14,15,16,17,18,19,20}
//0: Inlet solenoid valve
//1: HLT element
//2: HLT TEMP SENSOR
//3: HLT mixer
//4: HLT outlet valve
//5: Mash Mixer
//6: Mash temp sensor
//7: Recirculation Pump
//8: Kettle Element
//9: Kettle Exhause Fan
//10: Kettle Pump
//11: Kettle Inlet Valve
//12: 240v
//13: Box Fan
//14: Chiller Solenoid Valve
//15: Herms return temp sensor
//16: Kettle temp sensor
//17: HLT pressure sensor
//18: Herms pressure sensor
//19: Kettle pressure sensor
//20: AIR PUMP

int sadefault[] =     {0,0,0,0,0,0,0,0,0,0,0 ,0 ,1000 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,2 ,0 ,0 ,0 ,0 ,0 ,500 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 };
int storedactions[] = {0,0,0,0,0,0,0,0,0,0,0 ,0 ,1000 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,2 ,0 ,0 ,0 ,0 ,0 ,500 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 };
//POSITION            {0,1,2,3,4,5,6,7,8,9,10,11,12   ,13,14,15,16,17,18,19,20,21,22,23,24,25,26  ,27,28,29,30,31,32,33,34,35,36}
String actionnames[] = {
"0: HLT INLET VALVE STATE",
"1: HLT SET VOLUME",
"2: HLT VOLUME",
"3: HLT OVERVOLUME FLOAT SWITCH STATUS",
"4: HLT ELEMENT STATE",
"5: HLT UNDERVOLUME FLOAT SWITCH STATUS",
"6: HLT MIXER STATE",
"7: HLT OUTLET VALVE STATE",
"8: HLT SET TEMPERATURE VALUE",
"9: HLT TEMPERATURE",
"10: HLT AUTO CONTROL",
"11: HLT POWER LEVEL",
"12: HLT CYCLE TIME",
"13: MASH MIXER STATE",
"14: MASH TEMPERATURE VALUE",
"15: MASH SET TEMPERATURE VALUE",
"16: MASH water to add",
"17: MASH RECIRC PUMP SPEED",
"18: MASH RECIRC POST COIL TEMP",
"19: MASH RECIRC INITIAL SIGHTGLASS VOLUME",
"20: MASH RECIRC RUNNING SIGHTGLASS VOLUME",
"21: MASH CYCLE TIME",
"22: KETTLE SET TEMPERATURE VALUE",
"23: KETTLE TEMPERATURE",
"24: KETTLE POWER LEVEL",
"25: KETTLE ELEMENT STATE",
"26: KETTLE CYCLE TIME",
"27: KETTLE OVERVOLUME FLOAT SWITCH STATUS",
"28: KETTLE UNDERVOLUME FLOAT SWITCH STATUS",
"29: CHILLER VALVE STATE",
"30: CHILLER INPUT TEMP",
"31: CHILLER OUTLET TEMP",
"32: 240V SSR state",
"33: BOX FAN STATE",
"34: Master system on / off",
"35: Temperature units (0-C,1-F)",};
//36: AIR PUMP 

void setup()
{
  Serial.begin(9600); 
  for(int x=22;x<39;x++){    pinMode(x,OUTPUT);    digitalWrite(x,HIGH);  }
  for(int x=46;x<49;x++){    pinMode(x,OUTPUT);    digitalWrite(x,HIGH);  }
  lvlres = 45;
}

void loop(){
    cmd = command(); // get command from serial port (or check if command exists)
    cmd.toUpperCase(); // convert command to upper case for ease of analyzing
    if(cmd.length()>0){processcommand();} // if command exists, process command to make modifications storedactions array
    refreshactions(); // process storedactions array and make changes (turn on elements, valves, mixers etc...)
}

void processcommand(){
      if(cmd.length()==3 || cmd.length()==2){
        if(     cmd.substring(0,3)=="OFF"){Serial.println("System Off");storedactions[34]=0;}
        else if(cmd.substring(0,2)=="ON" ){Serial.println("Welcome to Brewbot, System activated");storedactions[34]=1;}
        else if(cmd.substring(0,2)=="13" ){Serial.print("Analog read pin A13: ");Serial.println(analogRead(A13));}
        else if(cmd.substring(0,2)=="14" ){Serial.print("Analog read pin A14: ");Serial.println(analogRead(A14));}
        else if(cmd.substring(0,2)=="15" ){Serial.print("Analog read pin A15: ");Serial.println(analogRead(A15));}
        else if(cmd.substring(0,2)=="SA" ){Serial.println("");for(int x = 0;x < 36; x++){Serial.print(actionnames[x]);Serial.print(" : ");Serial.println(storedactions[x]);}Serial.println("");}
        else{Serial.println("Command not recognized");}
      }
      else if (cmd.length() == 6 || cmd.length() == 7 || cmd.length() == 8){
        if(cmd.substring(0,3)=="HLT"){
            if(     cmd.substring(4,7)=="TMP"){Serial.print("HLT TEMPERATURE: "); int t = tempsensor(1);storedactions[9]=t;hlttemplast=millis();Serial.print(t/10); Serial.print(".");Serial.print(t-(t/10)*10);  if(storedactions[35]==1){Serial.println("F");}else{Serial.println("C");}}
            else if(cmd.substring(4,7)=="VOL"){Serial.print("HLT Volume: ");      Serial.print(storedactions[2]/10); Serial.print("."); Serial.print(storedactions[2]-(storedactions[2]/10)*10); Serial.println("L"); }          }

        else if(    cmd.substring(0,3)=="MSH"){
            if(     cmd.substring(4,7)=="TMP"){Serial.print("MASH TEMPERATURE: ");int t = tempsensor(2);storedactions[14]=t;mshtemplast=millis();Serial.print(t/10);Serial.print(".");Serial.print(t-(t/10)*10);if(storedactions[35]==1){Serial.println("F");}else{Serial.println("C");}}
            /*else if(cmd.substring(4,7)=="MIX"){Serial.print("MASH MIXER STATE: ");if(storedactions[13]==1){Serial.println("ON");}else{Serial.println("OFF");}}
            else if(cmd.substring(4,7)=="REC"){Serial.print("RECIRCULATION STATUS: ");if(storedactions[16]==1){Serial.println("ON");}else{Serial.println("OFF");}}*/      }
            
        /*else if(cmd.substring(0,3)=="KET"){
            if(cmd.substring(4,7)=="TMP"){Serial.print("KETTLE TEMPERATURE: ");int t = tempsensor(3);storedactions[23]=t;kettemplast=millis();Serial.print(t/100);Serial.print(".");Serial.print(t-(t/100)*100);Serial.println("C");}
            else if(cmd.substring(0,3)=="CTI"){Serial.print("KETTLE Cycle time: "); intx=atoi(cmd.substring(4,8).c_str()); Serial.print(intx); Serial.println("ms"); storedactions[26]=intx;}}
        */
        else{Serial.println("Command not recognized");}}
      
     else if (cmd.length() == 10 || cmd.length() == 11 || cmd.length() == 12){
        if(cmd.substring(0,3)=="HLT"){
            if(     cmd.substring(4,7)=="TMP"){intx=atoi(cmd.substring(8,cmd.length()).c_str()); storedactions[8]=intx; Serial.print("HLT tmp to "); Serial.print(intx/10);Serial.print(".");Serial.print(intx-(intx/10)*10); if(storedactions[35]==0){Serial.println("C");}else{Serial.println("F");}}
            else if(cmd.substring(4,7)=="VOL"){intx=atoi(cmd.substring(8,cmd.length()).c_str()); storedactions[1]=intx; Serial.print("HLT vol to "); Serial.print(intx/10);Serial.print(".");Serial.print(intx-(intx/10)*10); Serial.println("L");}
            else if(cmd.substring(4,7)=="ATS"){if(cmd.substring(8,10)=="ON"){storedactions[10]=1; tempvol = storedactions[1]; storedactions[1] = storedactions[1] + 2;}else if(cmd.substring(8,11)=="OFF"){storedactions[10]=0;}}
            /*else if(cmd.substring(4,7)=="IVL"){if(cmd.substring(8,10)=="ON"){storedactions[0]=1;}else if(cmd.substring(8,11)=="OFF"){storedactions[0]=0;}}
            else if(cmd.substring(4,7)=="MIX"){if(cmd.substring(8,10)=="ON"){storedactions[6]=1;}else if(cmd.substring(8,11)=="OFF"){storedactions[6]=0;}}
            else if(cmd.substring(4,7)=="OVL"){if(cmd.substring(8,10)=="ON"){storedactions[7]=1;}else if(cmd.substring(8,11)=="OFF"){storedactions[7]=0;}}        
            else if(cmd.substring(4,7)=="POW"){intx=atoi(cmd.substring(8,11).c_str()); Serial.print("HLT POWER to "); Serial.print(intx); Serial.println("%"); storedactions[11]=intx; storedactions[24]=0;}
            else if(cmd.substring(4,7)=="CTI"){intx=atoi(cmd.substring(4,8).c_str()); Serial.print("HLT cycle time to "); Serial.print(intx); Serial.println("ms"); storedactions[12]=intx;} 
            else if(cmd.substring(4,11)=="LVL RST"){lvlres = analogRead(13);}
            else if(cmd.substring(4,7)=="ELE"){if(cmd.substring(8,10)=="ON"){storedactions[4]=1;}else if(cmd.substring(8,11)=="OFF"){storedactions[4]=0;}}*/
            else{Serial.println("Command not recognized");}}
        else if(cmd.substring(0,3)=="MSH"){
            if(     cmd.substring(4,7)=="TMP"){intx=atoi(cmd.substring(8,cmd.length()).c_str()); Serial.print("MASH temp to "); Serial.print(intx); Serial.println("C"); storedactions[9]=intx;}
            else if(cmd.substring(4,7)=="ADD"){intx=atoi(cmd.substring(8,cmd.length()).c_str()); Serial.print(intx); Serial.println("L of water will be added to mash"); storedactions[16]=intx-2; mashwateradd = storedactions[16]; ihlv = storedactions[2];}
            //else if(cmd.substring(4,7)=="REC"){if(cmd.substring(8,10)=="ON"){storedactions[16]=1;}else if(cmd.substring(8,11)=="OFF"){storedactions[16]=0;}}
            //else if(cmd.substring(4,7)=="CTI"){intx=atoi(cmd.substring(4,8).c_str()); Serial.print("MASH cycle time to "); Serial.print(intx); Serial.println("ms"); storedactions[21]=intx;} 
            //else if(cmd.substring(4,7)=="MIX"){if(cmd.substring(8,10)=="ON"){storedactions[13]=1;}else if(cmd.substring(8,11)=="OFF"){storedactions[13]=0;}}     
            else{Serial.println("Command not recognized");}}  
        else if(cmd.substring(0,3)=="KET"){
            if(     cmd.substring(4,7)=="TMP"){intx=atoi(cmd.substring(8,cmd.length()).c_str()); Serial.print("Kettle temp to "); Serial.print(intx); Serial.println("C"); storedactions[10]=intx;}
            else if(cmd.substring(4,7)=="POW"){intx=atoi(cmd.substring(8,cmd.length()).c_str()); Serial.print("KETTLE POWER to "); Serial.print(intx); Serial.println("%"); storedactions[24]=intx; storedactions[10]=0;}            
            //else if(cmd.substring(4,7)=="CTI"){intx=atoi(cmd.substring(4,8).c_str()); Serial.print("MASH cycle time to "); Serial.print(intx); Serial.println("ms"); storedactions[26]=intx;}
            //else if(cmd.substring(4,7)=="REC"){if(cmd.substring(8,10)=="ON"){Serial.println("KETTLE PUMP ON");storedactions[25]=1;}else if(cmd.substring(8,11)=="OFF"){Serial.println("KETTLE PUMP OFF");storedactions[25]=0;}}        
            else{Serial.println("Command not recognized");}}
            
        //else if(cmd.substring(0,10)=="SYS STATUS"){for(int x = 0; x<35; x++){}}
        else{Serial.println("Command not recognized");}
      }
      else if (cmd.substring(0,4) == "HELP"){
        Serial.println("Command choices are in format XXX or XXX YYY or XXX YYY ZZZ");
        Serial.println("XXX choices are either 'on' or 'off'");
        Serial.println("XXX YYY will respond with temperatures or state of device");
        Serial.println("XXX YYY ZZZ will allow you to modify the state of a device");
        Serial.println("XXX choices are HLT, MSH and KET");
        Serial.println("YYY(ZZZ) choices are: (HLT)->TMP(xxxx),IVL(ON,OFF),MIX(ON,OFF),OVL(ON,OFF),ATS(ON,OFF)");
        Serial.println("YYY(ZZZ) choices are: (MSH)->TMP(XXXx),REC(ON,OFF),MIX(ON,OFF)");
        Serial.println("YYY(ZZZ) choices are: (KET)->TMP(XXXx),REC(ON,OFF),POW(XXX)");
      }
}

void refreshactions(){

    for(int x = 0; x<37
    ; x++){//run through each position in the storedactions array and make the changes that the array states.
        if(x == 0){if(storedactions[0] == 0){digitalWrite(componentlocation[0],HIGH);}else if(storedactions[0] == 1){digitalWrite(componentlocation[0],LOW);}}
        else if(x == 1){//hlt set volume
            if(storedactions[1]>450){storedactions[1]=450; Serial.println("HLT set volume over 45L, reduced to max of 45L");}}
        else if(x == 2){//hlt actual volume
            if(millis()-hltvollast > 500){
                  if(hltvolsum / hltvolcount < 98){storedactions[2] = 30;}
                  else{storedactions[2] = 30 + ((hltvolsum/hltvolcount - 98)*50/38); }
                  hltvolcount = 0;               hltvolsum = 0;                  hltvollast=millis();}
            else{hltvolsum += analogRead(13);  hltvolcount += 1;}          }
        else if(x == 3){}//hlt overvolume float switch status
        else if(x == 4){//hlt element state
             if(storedactions[4] == 0){digitalWrite(componentlocation[1],HIGH);}else if(storedactions[4] == 1){digitalWrite(componentlocation[1],LOW);}}
        else if(x == 5){}//hlt undervolume float switch status 
        else if(x == 6){//HLT Mixer
            if(storedactions[6] == 0){digitalWrite(componentlocation[3],HIGH);}else if(storedactions[6] == 1){digitalWrite(componentlocation[3],LOW);}}
        else if(x == 7){//HLT outlet valve
            if(storedactions[7] == 0){digitalWrite(componentlocation[4],HIGH);}else if(storedactions[7] == 1){digitalWrite(componentlocation[4],LOW);}}
        else if(x == 8){}//HLT set temperature value
        else if(x == 9){}//HLT temperature
        else if(x == 10){
            if(storedactions[10]==1){//REFRESH HLT TEMPERATURE AND SET HLT POWER LEVEL
                if(storedactions[1]>storedactions[2]){storedactions[0]=1;}else{storedactions[0]=0;storedactions[1]=tempvol;}//turn on water inlet if water level is too low
                if(millis()-hlttemplast>15000 || storedactions[9]==0){storedactions[9] = tempsensor(1);hlttemplast=millis();
                    Serial.print("HLT ");Serial.print(storedactions[2]/10);Serial.print(".");Serial.print(storedactions[2]-(storedactions[2]/10)*10);Serial.print("L @ "); Serial.print(storedactions[9]/10);Serial.print(".");Serial.print(storedactions[9]-(storedactions[9]/10)*10);if(storedactions[35]==0){Serial.println("C");}else{Serial.println("F");}} //every 15 seconds update temperature of hlt    
                if(storedactions[8]-storedactions[9]>40){                                                 storedactions[11]=100;} 
                else if(storedactions[8]-storedactions[9]<20 && storedactions[8]-storedactions[9]>=10){   storedactions[11]=50;}
                else if(storedactions[8]-storedactions[9]<10 && storedactions[8]-storedactions[9]>=0){    storedactions[11]=25;}
                else if(storedactions[8]-storedactions[9]<0){                                             storedactions[11]=0;}}
            else if(storedactions[10]==0){     storedactions[0]=0;                                        storedactions[11]=0;}}
        else if(x == 11){ //HOT LIQUOR TANK POWER LEVEL
            if(storedactions[11]>0){
                storedactions[24]=0; digitalWrite(componentlocation[10],HIGH);//Ensure Kettle power is off before turning on HLT power
                if(storedactions[4]==1){if((millis()-hltssron)>( long(storedactions[12])*long(storedactions[11])/100)){storedactions[4]=0; hltssroff=millis();}}
                else if(storedactions[4]==0){if((millis()-hltssroff)>(storedactions[12]-(long(storedactions[12])*long(storedactions[11])/100))){storedactions[4]=1;hltssron=millis();}}}
            else{storedactions[4]=0;}} 
        else if(x == 12){}//HLT cycle time  
        else if(x == 13){//mash mixer
            if(storedactions[13] == 0){digitalWrite(componentlocation[5],HIGH);}else if(storedactions[13] == 1){digitalWrite(componentlocation[5],LOW);}}
        else if(x == 14){}//Mash temperature       
        else if(x == 15){}//Mash set temperature        
        else if(x == 16){ //Mash water to add
            if(storedactions[16] > 0){
              if(storedactions[7]==0){digitalWrite(27,LOW);delay(2000);}
              storedactions[10]=0;  storedactions[11]=0; storedactions[0]=0; storedactions[4]=0; // turn off hlt ats control so the element and the inlet valve stay closed during this operation
              if(millis() - mashwaterlast > 1000){
                  storedactions[16] = mashwateradd - (ihlv - storedactions[2]); // refresh the amount of water left to add to the mash
                  storedactions[7] = 1;
                  mashwaterlast = millis();
                  Serial.print(storedactions[16]/10); Serial.print("."); Serial.print(storedactions[16]-(storedactions[16]/10)*10); Serial.println("L of water left");
                  if(storedactions[2]==30){storedactions[16]=0;Serial.println("Reached limit (3L left) of automated water measurement");}
            }}
            else{storedactions[7]=0;}}
        else if(x == 17){}//Mash recirculation pump speed
        else if(x == 18){}//Mash recirculation post coil temp
        else if(x == 19){}//MASH RECIRC INITIAL SIGHTGLASS VOLUME
        else if(x == 20){}//MASH RECIRC RUNNING SIGHTGLASS VOLUME
        else if(x == 21){}//MASH CYCLE TIME
        else if(x == 22){}//KETTLE SET TEMPERATURE VALUE (CELCIUS)
        else if(x == 23){}//KETTLE TEMPERATURE  
        else if(x == 24){  //KETTLE POWER LEVEL          
            if(storedactions[24]>0){
                storedactions[10]=0; storedactions[4]=0; digitalWrite(componentlocation[1],HIGH);//Ensure HLT power is off before turing on Kettle Power
                if(storedactions[25]==1){if((millis()-ketssron)>(long(storedactions[26])*long(storedactions[24])/100)){storedactions[25]=0; ketssroff=millis();}}
                else{if((millis()-ketssroff)>(storedactions[26]-(long(storedactions[26])*long(storedactions[24])/100))){storedactions[25]=1;ketssron=millis();}}            }
            else{storedactions[25]=0;}}
        else if(x == 25){if(storedactions[25] == 0){digitalWrite(componentlocation[8],HIGH); }else if(storedactions[25] == 1){digitalWrite(componentlocation[8],LOW);}}
        else if(x == 26){}  //KETTLE CYCLE TIME
        else if(x == 27){}  //KETTLE OVERVOLUME FLOAT SWITCH STATUS
        else if(x == 28){}  //KETTLE UNDERVOLUME FLOAT SWITCH STATUS
        else if(x == 29){   //chiller valve state
            if(storedactions[29] == 0){digitalWrite(componentlocation[14],HIGH);}else if(storedactions[29] == 1){digitalWrite(componentlocation[14],LOW);}}
        else if(x == 30){}  //chiller input temp
        else if(x == 31){}  //chiller output temp
        else if(x == 32){   //If hlt or kettle element ats states are on
            if(storedactions[24] > 0 || storedactions[11] > 0){digitalWrite(componentlocation[12],LOW);}else{digitalWrite(componentlocation[12],HIGH);}}
        else if(x == 33){   //If hlt or kettle elements are on, turn box fan on
            if(storedactions[24] > 0 || storedactions[11] > 0){digitalWrite(componentlocation[13],LOW);}else{digitalWrite(componentlocation[13],HIGH);}}
        else if(x == 34){ if(storedactions[34]==0){for(int x=0;x<35;x++){storedactions[x] = sadefault[x];}}}
        else if(x == 36){ if(storedactions[10] == 1 || storedactions[16] > 0){digitalWrite(27,LOW);}else{digitalWrite(27,HIGH);}}        
      }
}



String command(){
  String tcmd = "";
  char tempchar;
  while(Serial.available()>0){
    delay(1);
    tempchar = Serial.read();
    tcmd += String(tempchar);
  }
  return tcmd;
}

int tempsensor(int x){
        if(x == 1)     {x=componentlocation[2];}
        else if(x == 2){x=componentlocation[6];}
        else if(x == 3){x=componentlocation[15];}
        else if(x == 4){x=componentlocation[16];}
        else{x=39;}
        
        OneWire ds(x);
        
        byte i;  byte present = 0;  byte type_s;  byte data[12];  byte addr[8];  float celsius, fahrenheit;  
        if ( !ds.search(addr)) {    ds.reset_search();  }
        switch (addr[0]) {    case 0x10:      type_s = 1;      break;
          case 0x28:      type_s = 0;      break;    
          case 0x22:      type_s = 0;      break;  } 
        ds.reset();  ds.select(addr);  ds.write(0x44,1);         // start conversion, with parasite power on at the end
        delay(750);     // maybe 750ms is enough, maybe not
        present = ds.reset();  ds.select(addr);    ds.write(0xBE);         // Read Scratchpad
        for ( i = 0; i < 9; i++) {    data[i] = ds.read();  }
        unsigned int raw = (data[1] << 8) | data[0];
        if (type_s) {    raw = raw << 3;    if (data[7] == 0x10) { raw = (raw & 0xFFF0) + 12 - data[6];}} else {
        byte cfg = (data[4] & 0x60);    if (cfg == 0x00) raw = raw << 3; else if (cfg == 0x20) raw = raw << 2;
        else if (cfg == 0x40) raw = raw << 1; }
        celsius = (float)raw / 16.0;
        fahrenheit = celsius * 1.8 + 32.0;
        if(storedactions[35]==0){return(celsius*10);}else{return(fahrenheit*10);}
}
