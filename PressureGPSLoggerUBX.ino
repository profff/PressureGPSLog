
// include the SD library:
#include <SPI.h>
#include <SD.h>
#include <XMLFileWriter.h>

//#define __KML
//#define __GPX
#define __CSV

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = BUILTIN_SDCARD;
File logfile;
String filename;

// Pressure Sensor pin
int PressureSensorPin = 0; 
#define PressureFactor 0.008053691
#define PressureOffset -1.409395973

//GPS definition
#include "Ubx.h"
// a uBlox object, which is on hardware
// serial port 1 with a baud rate of 115200
UBLOX gps(Serial1,115200);
 
int led = 13;

void SDCardSetup()
{
   Serial.print("\nInitializing SD card...");

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    while (1);
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }
}

void SDCardInfo()
{
  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);

}

void SDCardFileList()
{
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
} 

int SDSetup()
{
  Serial.print("Initializing SD card: ");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return 1;
  }
  Serial.println("card initialized.");
  return 0;
}

void SDLogFileOpen()
{
  //filename = filename+gps.date.year()+gps.date.month()+gps.date.day();
  //filename = filename+"_"+gps.time.hour()+gps.time.minute()+gps.time.second();
  int i=0;
  String FileExt="";
#ifdef __CSV  
  FileExt=".CSV";
#endif 
#ifdef __GPX
  FileExt=".GPX";
#endif 
#ifdef __KML
  FileExt=".KML";
#endif   
    filename=i+FileExt;
    // create if does not exist, do not open existing, write, sync after write
    while (SD.exists(filename.c_str())) {
      i++;
        filename=i+FileExt;
    }
    // test sd full
  logfile = SD.open(filename.c_str(), FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create "); 
    Serial.println(filename.c_str());
    while(1);
  }
  logfile.close();
  
  if(i) // if it exists read last log file
    {
    String st=(i-1)+FileExt;
    logfile = SD.open(st.c_str());
    while (logfile.available()) {
      Serial.write(logfile.read());
    }  
    logfile.close();
  }

  Serial.print("Writing to "); 
  Serial.println(filename.c_str());
#ifdef __CSV
  logfile = SD.open(filename.c_str(), FILE_WRITE);
  logfile.println("DATE,TIME,LATITUDE,LONGITUDE,ALTITUDE,PRESSURE");
  logfile.close();
#endif
  
#ifdef __GPX
  logfile = SD.open(filename.c_str(), FILE_WRITE);
  logfile.println("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>");
  logfile.println("<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" creator=\"byHand\" version=\"1.1\""); 
  logfile.println("\txmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""); 
  logfile.println("\txsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">");
  logfile.println("<trk>");
  logfile.println("\t<name></name>");
  logfile.println("\t<src>DRONEVOLT presure gps logger</src>");
  logfile.println("\t<number>1</number>");
  logfile.println("\t<trkseg>");
  logfile.println("\t</trkseg>");
  logfile.println("</trk>");
  logfile.println("</gpx>");
  logfile.close();
#endif
#ifdef __KML
  logfile = SD.open(filename.c_str(), FILE_WRITE);
  logfile.println(F("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
  
  logfile.println(F("<!--DroneVolt Pressure GPS logger-->")); 
  logfile.println(F("<kml xmlns=\"http://earth.google.com/kml/2.0\">"));
  
  logfile.println(F("  <Document>"));
  logfile.println(F("    <name>Paths</name>"));
  logfile.println(F("    <style id=\"yellowLineGreenPoly\">"));

  logfile.println(F("      <LineStyle>"));
  logfile.println(F("        <color>7F00FFFF</color>"));
  logfile.println(F("        <width>4</width>"));
  logfile.println(F("      </LineStyle>"));

  logfile.println(F("      <PolyStyle>"));
  logfile.println(F("        <color>7F00FF00</color>"));
  logfile.println(F("      </PolyStyle>"));
  logfile.println(F("    </style>"));
  
  logfile.println(F("    <Placemark>"));
  logfile.println(F("      <name>Absolute Extruded</name>"));
  logfile.println(F("      <description></description>"));
  logfile.println(F("      <styleUrl>#yellowLineGreenPoly</styleUrl>"));
  logfile.println(F("      <LineString>"));
  logfile.println(F("        <extrude>1</extrude>"));
  logfile.println(F("        <tessellate>1</tessellate>"));
  logfile.println(F("        <altitudeMode>absolute</altitudeMode>"));
  logfile.println(F("        <coordinates>"));
  logfile.println(F("        </coordinates>"));
  logfile.println(F("      </LineString>"));
  logfile.println(F("    </Placemark>"));
  
  logfile.println(F("  </Document>"));
  logfile.println(F("</kml>"));
  logfile.close();
#endif

}

int GPSSetup()
{
  Serial.print("\nGPS setup : [");
  gps.begin();
  Serial.println(F("GPS data OK"));
  
  return 0; 
}

double PressureRead()
{
  int sensor = analogRead(PressureSensorPin);
  double res = PressureFactor*sensor+PressureOffset;
  return res;
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  /* while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }*/
  pinMode(led, OUTPUT);  
  GPSSetup(); 
  
  SDCardSetup();
  SDCardInfo();
  SDCardFileList();

 if(SDSetup())
   ;// while(1); // SD error

 SDLogFileOpen();
}

void loop(void) {
  static double lastlat=gps.getLatitude_deg();
  static double lastlng=gps.getLongitude_deg();
  static double lastalt=gps.getMSLHeight_ft()/3.28084;
  int footeroffset=0;
  if(gps.readSensor())
  {
  digitalWrite(led, !digitalRead(led)); 
    //if((lastlat!=gps.getLatitude_deg())&&(lastlng!=gps.getLongitude_deg()))
    {
      String StrToWrite="";
      char szdate[32];
      char sztime[32];
      
      lastlat=gps.getLatitude_deg();
      lastlng=gps.getLongitude_deg();
      lastalt=gps.getMSLHeight_ft()/3.28084; // convert in m
      double pressure=PressureRead();
           
      sprintf(szdate, "%04d-%02d-%02d", gps.getYear(), gps.getMonth(), gps.getDay());
      sprintf(sztime, "%02d:%02d:%02d", gps.getHour(), gps.getMin(), gps.getSec());
     
      Serial.print(szdate);
      Serial.print(" ");
      Serial.print(sztime);
      Serial.print("\t");
      Serial.print("lat: ");
      Serial.print(lastlat,6);
      Serial.print(" lon: ");
      Serial.print(lastlng,6);
      Serial.print(" alt: ");
      Serial.print(lastalt,2);
            

      Serial.print("\t bar: ");
      Serial.print(pressure,6);
      Serial.print("\n");
#ifdef __CSV
      logfile = SD.open(filename.c_str(), FILE_WRITE);
      logfile.print(szdate);
      logfile.print(",");
      logfile.print(sztime);
      logfile.print(",");

      logfile.print(lastlat,6);
      logfile.print(",");
      logfile.print(lastlng,6);
      logfile.print(",");
      logfile.print(lastalt,6);
  
      logfile.print(",");
      logfile.print(pressure,6);
      logfile.print("\n");
      logfile.close();
#endif    
   
#ifdef __GPX 
      logfile = SD.open(filename.c_str(), O_READ | O_WRITE);
      footeroffset=25;
      
      logfile.seek(logfile.size()-footeroffset);
      
      logfile.print("\t\t<trkpt lat=");
      logfile.print(lastlat,6);
      logfile.print(" lon=");
      logfile.print(lastlng,6);
      logfile.print(">\n");
      
      logfile.print("\t\t\t<ele>");
      logfile.print(lastalt,6);
      logfile.print("</ele>\n");      
      
      logfile.print("\t\t\t<time>");
      logfile.print(szdate);
      logfile.print("T");
      logfile.print(sztime);
      logfile.print("Z</time>\n");
      
      logfile.print("\t\t\t<extension>\n\t\t\t\t<pressure>");
      logfile.print(pressure,6);
      logfile.print("</pressure>\n\t\t\t</extension>\n");
      logfile.print("\t\t</trkpt>\n");
      // footer
      logfile.println("\t</trkseg>");
      logfile.println("</trk>");
      logfile.close();
#endif
#ifdef __KML
      logfile = SD.open(filename.c_str(), O_READ | O_WRITE);
      footeroffset=80;
      
      logfile.seek(logfile.size()-footeroffset);
      

      logfile.print(lastlng,10);
      logfile.print(",");
      logfile.print(lastlat,10);
      logfile.print(",");
      logfile.print(lastalt,2);
      logfile.print("\n");

      logfile.println(F("        </coordinates>"));
      logfile.println(F("      </LineString>"));
      logfile.println(F("    </Placemark>"));
      logfile.println(F("  </Document>"));
      logfile.println(F("</kml>"));

      logfile.close();
     
#endif 
    
    } 
  }  
  
}
