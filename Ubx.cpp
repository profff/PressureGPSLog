#include "Arduino.h"
#include "Ubx.h"

/* uBlox object, input the serial bus and baud rate */
UBLOX::UBLOX(HardwareSerial& bus,uint32_t baud)
{
  _bus = &bus;
	_baud = baud;
}

/* starts the serial communication */
void UBLOX::begin()
{
  // initialize parsing state
  _wr = 0;
  frameState=Header;
  // begin the serial port for uBlox
  _bus->begin(_baud);
}

/* reads packets from the uBlox receiver */
bool UBLOX::readSensor()
{
  if (_parse(0,0,0))
    {
      if((_validPreambule->msg_class==0x01)&&(_validPreambule->msg_id==0x07))
	{
	  _validPayload=(struct _UBX_NAV_PVT_payload*)msg_payload;
	  return true;
	}
    }
  return false;
}

/* parse the uBlox data */
bool UBLOX::_parse(uint8_t msg_class,uint8_t msg_id,uint16_t msg_length)
{
  // read a byte from the serial port
  while (_bus->available())
    {
      _byte = _bus->read();
      // dbug.print(_byte,HEX);
      // dbug.print(" ");
      staticRxBuff[_wr]=_byte;
      _wr++;_wr%=107;

      switch(frameState)
	{
	case Header:
	  {
	    if(_byte!=_ubxHeader[_wr-1])
	      _wr=0;
      
	    if(_wr>=HEADER_LENGTH)
	      {
		// dbug.print("header: ");
		//dbug.print(_wr);
		// dbug.print("\t");
		// dbug.print(staticRxBuff[0],HEX);
		// dbug.print(" ");
		// dbug.println(staticRxBuff[1],HEX);
		frameState=Preambule;
	      }
	  }
	  break;
	  
	case Preambule:
	  {
	    if(_wr>=HEADER_LENGTH+PREAMBULE_LENGTH)
	      {
		msg_preambule=(staticRxBuff+HEADER_LENGTH);
		_validPreambule=(struct _UBX_PREAMBULE*)(staticRxBuff+HEADER_LENGTH);
		
		// dbug.print("preambule: ");
		//dbug.print(_wr);
		// dbug.print("\t");
		// dbug.print(_validPreambule->msg_class,HEX);
		// dbug.print(" ");
		// dbug.print(_validPreambule->msg_id,HEX);
		// dbug.print(" ");
		// dbug.println(_validPreambule->msg_length);
		frameState=Payload;
		if(_validPreambule->msg_length>=100)
		  {
		    // dbug.println(" buff too short ");
		    _wr=0; frameState=Header;
		  }
	      }
	  }
	  break;
	case Payload:
	  {
	    if(_wr>=HEADER_LENGTH+PREAMBULE_LENGTH+_validPreambule->msg_length)
	      {
		msg_payload=(staticRxBuff+HEADER_LENGTH+PREAMBULE_LENGTH);
		int i=0;
		// dbug.print("payload: ");
		//dbug.print(_wr);
		//dbug.print("\t");
		while(i<_validPreambule->msg_length)
		  {
		    // dbug.print(staticRxBuff[HEADER_LENGTH+PREAMBULE_LENGTH+i],HEX);
		    // dbug.print(" ");
		    i++;
		  }
		frameState=Chksum;
	      }
	  }
	  break;
	case Chksum:
	  {
	    if(_wr>=HEADER_LENGTH+PREAMBULE_LENGTH+_validPreambule->msg_length+CHKSUM_LENGTH)
	      {
		uint8_t chk[2]={0x55,0x55};
		msg_chksum=(staticRxBuff+HEADER_LENGTH+PREAMBULE_LENGTH+_validPreambule->msg_length);
		_calcChecksum(chk,msg_preambule,PREAMBULE_LENGTH+_validPreambule->msg_length);
		// dbug.print("\nchksum's :");
		//dbug.print(_wr);
		// dbug.print("\n");
		// dbug.print("frame chksum :\t");
		// dbug.print(msg_chksum[0],HEX);
		// dbug.print(" ");
		// dbug.println(msg_chksum[1],HEX);
		// dbug.print("calc chksum :\t");
		// dbug.print(chk[0],HEX);
		// dbug.print(" ");
		// dbug.println(chk[1],HEX);
		
		_wr=0; frameState=Header;
		if((chk[0]!=msg_chksum[0])||(chk[1]!=msg_chksum[1]))
		  return false;
		// dbug.print(" chksum err\n");
		// else
		//dbug.print("\n");
		// memcpy(&_validPayload,msg_payload,sizeof(_validPayload));
		return true;
	      }
	  }
	  break;
	}
    }
  return false;
}


/* uBlox checksum */
void UBLOX::_calcChecksum(uint8_t* CK, uint8_t* payload, int size)
{
  CK[0] = 0;
  CK[1] = 0;
  uint8_t CK0=0;
  uint8_t CK1=0;
  uint8_t i;
  i=0;
  while (i < size)
    {
      CK0 += payload[i];
      CK1 += CK0;
      i++;
    }
  CK[0]=CK0;
  CK[1]=CK1;
}


// public fx
/* GPS time of week of nav solution, ms */
uint32_t UBLOX::getTow_ms()
{
	return _validPayload->iTOW;
}

/* UTC year */
uint16_t UBLOX::getYear()
{
	return _validPayload->year;
}

/* UTC month */
uint8_t UBLOX::getMonth()
{
	return _validPayload->month;
}

/* UTC day */
uint8_t UBLOX::getDay()
{
	return _validPayload->day;
}

/* UTC hour */
uint8_t UBLOX::getHour()
{
	return _validPayload->hour;
}

/* UTC minute */
uint8_t UBLOX::getMin()
{
	return _validPayload->min;
}

/* UTC second */
uint8_t UBLOX::getSec()
{
	return _validPayload->sec;
}

/* UTC fraction of a second, ns */
int32_t UBLOX::getNanoSec()
{
	return _validPayload->nano;
}

/* number of satellites used in nav solution */
uint8_t UBLOX::getNumSatellites()
{
	return _validPayload->numSV;
}

/* longitude, deg */
double UBLOX::getLongitude_deg()
{
	return (double)_validPayload->lon * 1e-7;
}

/* latitude, deg */
double UBLOX::getLatitude_deg()
{
	return (double)_validPayload->lat * 1e-7;
}

/* height above the ellipsoid, ft */
double UBLOX::getEllipsoidHeight_ft()
{
	return (double)_validPayload->height * 1e-3 * _m2ft;
}

/* height above mean sea level, ft */
double UBLOX::getMSLHeight_ft()
{
	return (double)_validPayload->hMSL * 1e-3 * _m2ft;
}

/* horizontal accuracy estimate, ft */
double UBLOX::getHorizontalAccuracy_ft()
{
	return (double)_validPayload->hAcc * 1e-3 * _m2ft;
}

/* vertical accuracy estimate, ft */
double UBLOX::getVerticalAccuracy_ft()
{
	return (double)_validPayload->vAcc * 1e-3 * _m2ft;
}

/* NED north velocity, ft/s */
double UBLOX::getNorthVelocity_fps()
{
	return (double)_validPayload->velN * 1e-3 * _m2ft;
}

/* NED east velocity, ft/s */
double UBLOX::getEastVelocity_fps()
{
	return (double)_validPayload->velE * 1e-3 * _m2ft;
}

/* NED down velocity ft/s */
double UBLOX::getDownVelocity_fps()
{
	return (double)_validPayload->velD * 1e-3 * _m2ft;
}

/* 2D ground speed, ft/s */
double UBLOX::getGroundSpeed_fps()
{
	return (double)_validPayload->gSpeed * 1e-3 * _m2ft;
}

/* speed accuracy estimate, ft/s */
double UBLOX::getSpeedAccuracy_fps()
{
	return (double)_validPayload->sAcc * 1e-3 * _m2ft;
}

/* 2D heading of motion, deg */
double UBLOX::getMotionHeading_deg()
{
	return (double)_validPayload->headMot * 1e-5;
}

/* 2D vehicle heading, deg */
double UBLOX::getVehicleHeading_deg()
{
	return (double)_validPayload->headVeh * 1e-5;
}

/* heading accuracy estimate, deg */
double UBLOX::getHeadingAccuracy_deg()
{
	return (double)_validPayload->headAcc * 1e-5;
}

/* magnetic declination, deg */
float UBLOX::getMagneticDeclination_deg()
{
	return (float)_validPayload->magDec * 1e-2;
}

/* magnetic declination accuracy estimate, deg */
float UBLOX::getMagneticDeclinationAccuracy_deg()
{
	return (float)_validPayload->magAcc * 1e-2;
}

/* longitude, rad */
double UBLOX::getLongitude_rad()
{
	return (double)_validPayload->lon * 1e-7 * _deg2rad;
}

/* latitude, rad */
double UBLOX::getLatitude_rad()
{
	return (double)_validPayload->lat * 1e-7 * _deg2rad;
}

/* height above the ellipsoid, m */
double UBLOX::getEllipsoidHeight_m()
{
	return (double)_validPayload->height * 1e-3;
}

/* height above mean sea level, m */
double UBLOX::getMSLHeight_m()
{
	return (double)_validPayload->hMSL * 1e-3;
}

/* horizontal accuracy estimate, m */
double UBLOX::getHorizontalAccuracy_m()
{
	return (double)_validPayload->hAcc * 1e-3;
}

/* vertical accuracy estimate, m */
double UBLOX::getVerticalAccuracy_m()
{
	return (double)_validPayload->vAcc * 1e-3;
}

/* NED north velocity, m/s */
double UBLOX::getNorthVelocity_ms()
{
	return (double)_validPayload->velN * 1e-3;
}

/* NED east velocity, m/s */
double UBLOX::getEastVelocity_ms()
{
	return (double)_validPayload->velE * 1e-3;
}

/* NED down velocity, m/s */
double UBLOX::getDownVelocity_ms()
{
	return (double)_validPayload->velD * 1e-3;
}

/* 2D ground speed, m/s */
double UBLOX::getGroundSpeed_ms()
{
	return (double)_validPayload->gSpeed * 1e-3;
}

/* speed accuracy estimate, m/s */
double UBLOX::getSpeedAccuracy_ms()
{
	return (double)_validPayload->sAcc * 1e-3;
}

/* 2D heading of motion, rad */
double UBLOX::getMotionHeading_rad()
{
	return (double)_validPayload->headMot * 1e-5 * _deg2rad;
}

/* 2D vehicle heading, rad */
double UBLOX::getVehicleHeading_rad()
{
	return (double)_validPayload->headVeh * 1e-5 * _deg2rad;
}

/* heading accuracy estimate, rad */
double UBLOX::getHeadingAccuracy_rad()
{
	return (double)_validPayload->headAcc * 1e-5 * _deg2rad;
}

/* magnetic declination, rad */
float UBLOX::getMagneticDeclination_rad()
{
	return (float)_validPayload->magDec * 1e-2 * _deg2rad;
}

/* magnetic declination accuracy estimate, rad */
float UBLOX::getMagneticDeclinationAccuracy_rad()
{
	return (float)_validPayload->magAcc * 1e-2 * _deg2rad;
}

/* position dilution of precision */
float UBLOX::getpDOP()
{
	return (float)_validPayload->pDOP * 1e-2;
}

/* fix type */
enum UBLOX::FixType UBLOX::getFixType()
{
	return (FixType)_validPayload->fixType;
}

/* power save mode */
enum UBLOX::PowerSaveMode UBLOX::getPowerSaveMode()
{
	return (PowerSaveMode)((_validPayload->flags >> 2) & 0x07);
}

/* carrier phase status */
enum UBLOX::CarrierPhaseStatus UBLOX::getCarrierPhaseStatus()
{
	return (CarrierPhaseStatus)((_validPayload->flags >> 6) & 0x03);
}

/* valid fix, within DOP and accuracy masks */
bool UBLOX::isGnssFixOk()
{
	return _validPayload->flags & 0x01;
}

/* differential corrections were applied */
bool UBLOX::isDiffCorrApplied()
{
	return _validPayload->flags & 0x02;
}

/* heading of vehicle is valid */
bool UBLOX::isHeadingValid()
{
	return _validPayload->flags & 0x20;
}

/* UTC date validity could be confirmed */
bool UBLOX::isConfirmedDate()
{
	return _validPayload->flags & 0x40;
}

/* UTC time validity could be confirmed */
bool UBLOX::isConfirmedTime()
{
	return _validPayload->flags & 0x80;
}

/* info about UTC date and time validity confirmation is available */
bool UBLOX::isTimeDateConfirmationAvail()
{
	return _validPayload->flags2 & 0x20;
}

/* valid UTC date */
bool UBLOX::isValidDate()
{
	return _validPayload->valid & 0x01;
}

/* valid UTC time */
bool UBLOX::isValidTime()
{
	return _validPayload->valid & 0x02;
}

/* UTC time of day has been fully resolved, no seconds uncertainty */
bool UBLOX::isTimeFullyResolved()
{
	return _validPayload->valid & 0x04;
}

/* valid magnetic declination estimate */
bool UBLOX::isMagneticDeclinationValid()
{
	return _validPayload->valid & 0x08;
}
