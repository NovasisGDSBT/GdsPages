#ifndef INFD_ICD_H
#define INFD_ICD_H
/*
     This file is autogenerated.
     ICD_DB_VERSION 1.0.0.0
     DEVICE_INTERFACE_VERSION 1.0.10.0
     GDB_TOOL_VERSION 5.10.5

*/

 // ComID constants
#define COMID_ICCUC_INFDIS ((unsigned long int)272310100)
#define COMID_IINFDISCTRLMAINT ((unsigned long int)272250100)
#define COMID_IINFDISCTRLOP ((unsigned long int)272200100)
#define COMID_OINFDISREPORT ((unsigned long int)272790100)


typedef struct {
	unsigned short int ICCUCLifeSign;
	unsigned short int Reserved1;
} CLifesign;


#define CCUC_INFDISLOADRESOURCE_RESERVED1_SIZE ((unsigned long int)3)
//const unsigned long int CCUC_INFDISLOADRESOURCE_RESERVED1_SIZE = 3;
typedef struct {
	unsigned char IInfdMode;
	unsigned char Reserved1[CCUC_INFDISLOADRESOURCE_RESERVED1_SIZE];
} CCCUC_INFDISLoadResource;


#define CCUC_INFDISLOADRESOURCE_IURL_SIZE ((unsigned long int)100)
#define CCUC_INFDISLOADRESOURCE_RESERVED2_SIZE ((unsigned long int)408)
//const unsigned long int CCUC_INFDISLOADRESOURCE_IURL_SIZE = 100;
//const unsigned long int CCUC_INFDISLOADRESOURCE_RESERVED1_SIZE = 408;
typedef struct {
	unsigned char IURL[CCUC_INFDISLOADRESOURCE_IURL_SIZE];
	unsigned char Reserved1[CCUC_INFDISLOADRESOURCE_RESERVED2_SIZE];
} CCCUC_INFDISLoadResource2;


typedef struct {
	CLifesign Lifesign;
	CCCUC_INFDISLoadResource LoadResource;
	CCCUC_INFDISLoadResource2 LoadResource2;
} CCCUC_INFDIS;

#define COMMANDS_RESERVED2_SIZE ((unsigned long int)16)
//const unsigned long int COMMANDS_RESERVED2_SIZE = 16;
typedef struct {
	unsigned char CReset;
	unsigned char Reserved1;
	unsigned char Reserved2[COMMANDS_RESERVED2_SIZE];
} CCommands;

#define INFDCOUNTERCOMMANDS_RESERVED1_SIZE ((unsigned long int)41)
//const unsigned long int INFDCOUNTERCOMMANDS_RESERVED1_SIZE = 41;
typedef struct {
	unsigned char CDurationCounterReset;
	unsigned char CBackOnCounterReset;
	unsigned char CNormalStartsCounterReset;
	unsigned char CWdogResetsCounterReset;
	unsigned char CResetAllCounters;
	unsigned char Reserved1[INFDCOUNTERCOMMANDS_RESERVED1_SIZE];
} CINFDCounterCommands;

#define RUNTIMEPARAMETEROVERWRITE_RESERVED1_SIZE ((unsigned long int)18)
#define RUNTIMEPARAMETEROVERWRITE_RESERVED2_SIZE ((unsigned long int)46)
//const unsigned long int RUNTIMEPARAMETEROVERWRITE_RESERVED1_SIZE = 18;
//const unsigned long int RUNTIMEPARAMETEROVERWRITE_RESERVED2_SIZE = 46;
typedef struct {
	unsigned char Reserved1[RUNTIMEPARAMETEROVERWRITE_RESERVED1_SIZE];
	unsigned char Reserved2[RUNTIMEPARAMETEROVERWRITE_RESERVED2_SIZE];
} CRuntimeParameterOverwrite;


typedef struct {
	CCommands Commands;
	CINFDCounterCommands INFDCounterCommands;
	CRuntimeParameterOverwrite RuntimeParameterOverwrite;
} CINFDISCtrlMaint;

#define CTRLCOMMANDS_RESERVED1_SIZE ((unsigned long int)2)
#define CTRLCOMMANDS_RESERVED2_SIZE ((unsigned long int)13)
#define CTRLCOMMANDS_RESERVED3_SIZE ((unsigned long int)46)
//const unsigned long int CTRLCOMMANDS_RESERVED1_SIZE = 2;
//const unsigned long int CTRLCOMMANDS_RESERVED2_SIZE = 13;
//const unsigned long int CTRLCOMMANDS_RESERVED3_SIZE = 46;
typedef struct {
	unsigned char CShutdownCmd;
	unsigned char CBacklightCmd;
	unsigned char Reserved1[CTRLCOMMANDS_RESERVED1_SIZE];
	unsigned char Reserved2[CTRLCOMMANDS_RESERVED2_SIZE];
	unsigned char CSystemMode;
	unsigned char Reserved3[CTRLCOMMANDS_RESERVED3_SIZE];
} CCtrlCommands;


typedef struct {
	CCtrlCommands CtrlCommands;
} CINFDISCtrlOp;

#define STATUSDATA_RESERVED1_SIZE ((unsigned long int)61)
//const unsigned long int STATUSDATA_RESERVED1_SIZE = 61;
typedef struct {
	unsigned short int ISystemLifeSign;
	unsigned char ISystemMode;
	unsigned char ITestMode;
	unsigned char IBacklightStatus;
	unsigned char Reserved1[STATUSDATA_RESERVED1_SIZE];
} CStatusData;

#define CNTDATA_RESERVED1_SIZE ((unsigned long int)48)
//const unsigned long int CNTDATA_RESERVED1_SIZE = 48;
typedef struct {
	unsigned long int ITFTWorkTime;
	unsigned long int ITFTBacklight;
	unsigned long int ITFTPowerUp;
	unsigned long int ITFTWatchdog;
	unsigned char Reserved1[CNTDATA_RESERVED1_SIZE];
} CCntData;

#define COMMODULE_RESERVED2_SIZE ((unsigned long int)4)
//const unsigned long int COMMODULE_RESERVED2_SIZE = 4;
typedef struct {
	unsigned char Reserved1;
	unsigned char FTimeoutComMod;
	unsigned char Reserved2[COMMODULE_RESERVED2_SIZE];
} CCOMModule;

#define APPLICATIONMODULE_RESERVED1_SIZE ((unsigned long int)2)
//const unsigned long int APPLICATIONMODULE_RESERVED1_SIZE = 2;
typedef struct {
	unsigned char FWatchdogApiMod;
	unsigned char FApiMod;
	unsigned char Reserved1[APPLICATIONMODULE_RESERVED1_SIZE];
} CApplicationModule;

#define TFTFAULTS_RESERVED2_SIZE ((unsigned long int)44)
//const unsigned long int TFTFAULTS_RESERVED2_SIZE = 44;
typedef struct {
	unsigned char FBcklightFault;
	unsigned char FTempSensor;
	unsigned char FTempORHigh;
	unsigned char FTempORLow;
	unsigned char Reserved1;
	unsigned char FAmbLightSensor;
	unsigned char Reserved2[TFTFAULTS_RESERVED2_SIZE];
} CTFTFaults;


typedef struct {
	CCOMModule COMModule;
	CApplicationModule ApplicationModule;
	CTFTFaults TFTFaults;
} CDiagData;


typedef struct {
	CStatusData StatusData;
	CCntData CntData;
	CDiagData DiagData;
} CINFDISReport;

#endif
