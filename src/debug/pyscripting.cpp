#include "dosbox.h"
#ifdef C_DEBUG_SCRIPTING
#include <Python.h>
#include <string.h>
#include "vga.h"
#include "control.h"
#include "debug_api.h"
#include "setup.h"
#include "debug_inc.h"
#include <signal.h>
#include "bindings/_dbox.h"
#include "bindings/_break.h"

#include <algorithm>
#include <iterator>

bool dosboxUI = false;

char dasmstr[200];
char* PYTHON_Dasm(PhysPt ptr, Bitu eip, int &size)
{
	size = (int)DasmI386(dasmstr, ptr, eip, cpu.code.big);
	return dasmstr;
}

/*
void
python_setmemory(Bitu loc, std::string *mem)
{
	char buf[mem->length()];
	mem->copy(buf, mem->length());
	for (Bitu x = 0; x < mem->length(); x++) {
		mem_writeb_checked(loc+x, buf[x]);
	}
	return;
}

void
python_getvidmemory(Bit16u x, Bit16u y, Bit16u w, Bit16u h, Bit8u page, std::string *mem)
{
	mem->reserve( w*h );
	for (Bitu i = 0; i < w*h; i++) {
		Bit8u val;
		INT10_GetPixel(x+(i%w), y+(i/w), page, &val);
		//Bit8u val = mem_readb(PhysMake(0xa000,i));
		mem->push_back(val);
	}
}

void
python_setvidmemory(Bit16u x, Bit16u y, Bit16u w, Bit16u h, Bit8u page, std::string *mem)
{
	char buf[mem->length()];
	mem->copy(buf, mem->length());
	for (Bitu i = 0; i < w*h; i++) {
		INT10_PutPixel(x+(i%w), y+(i/w), page, buf[i]);
	}
}
void 
python_getpalette(std::string *pal)
{
	int count=255;
	pal->reserve(count*3);
	// copied from INT10_GetDACBlock which uses emulator memory
  IO_Write(VGAREG_DAC_READ_ADDRESS,(Bit8u)0);
  for (;count>0;count--) {
    pal->push_back(IO_Read(VGAREG_DAC_DATA));
    pal->push_back(IO_Read(VGAREG_DAC_DATA));
    pal->push_back(IO_Read(VGAREG_DAC_DATA));
  }
}

void 
python_setpalette(std::string *pal)
{
	char buf[pal->length()];
	pal->copy(buf, pal->length());

	int count=255, i=0;
  IO_Write(VGAREG_DAC_WRITE_ADDRESS,(Bit8u)0);
  for (;count>0;count--) {
  		//if ((real_readb(BIOSMEM_SEG,BIOSMEM_MODESET_CTL)&0x06)==0) {
      //Bit32u i=(( 77*red + 151*green + 28*blue ) + 0x80) >> 8;
      //Bit8u ic=(i>0x3f) ? 0x3f : ((Bit8u)(i & 0xff));
      IO_Write(VGAREG_DAC_DATA,buf[i++]);
      IO_Write(VGAREG_DAC_DATA,buf[i++]);
      IO_Write(VGAREG_DAC_DATA,buf[i++]);
  }
}

int python_vgamode() { return CurMode->mode; }
*/

void
python_insertvar(char *name, Bit32u addr)
{
	CDebugVar::InsertVariable(name, addr);
}

std::list<CDebugVar>
python_vars()
{
	std::list<CDebugVar> ret;
	std::list<CDebugVar*>::iterator i;
	for(i=CDebugVar::varList.begin(); i != CDebugVar::varList.end(); i++) {
		CDebugVar *var = (*i);
		ret.push_back(*var);
	}
	return ret;
}

bool PYTHON_Break(CBreakpoint *bp){
    break_run(bp);
    return false;
}

bool PYTHON_Command(const char *cmd)
{
    if (!cmd || !strncmp(cmd, "exit()", 6)){
        raise(SIGTERM);
        return true;
    }
    int res = PyRun_SimpleString(cmd);
    return res==0;
}

bool PYTHON_IsDosboxUI(void){
    return dosboxUI;
}

Bitu PYTHON_Loop(bool& dbui){
    dbui = dosboxUI;
    return (Bitu)dbox_loop();
}


void PYTHON_ShutDown(Section* sec){
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
    if (dosboxUI){
        DBGUI_ShutDown();
    }
}

void DEBUG_ShowMsg(char const* format,...){
    va_list msg;
    va_start(msg, format);
    if (dosboxUI){
        DEBUG_ShowMsgV(format, msg);
    }else{
        vprintf(format, msg);
        //printf("\n");
    }
    va_end(msg);
}

const char* getpchar(string&s){return s.c_str();}

void PYTHON_Init(Section* sec){
    sec->AddDestroyFunction(&PYTHON_ShutDown);
    Section_prop * sect=static_cast<Section_prop *>(sec);
    Py_Initialize();
    Py_InspectFlag = true;
    Py_SetProgramName((char*)"dosbox");

    //args 'n' working dir in python paths
    vector<string> args;
    args.push_back(control->cmdline->GetFileName());
    control->cmdline->FillVector(args);
    Property* prop;
    int i=0;
    while ((prop=sect->Get_prop(i++))){
        string p = "--" + prop->propname + "=";
        p += string(prop->GetValue());
        args.push_back(p);
    }

    vector<const char*> argv;
    std::transform(args.begin(), args.end(), back_inserter(argv), &getpchar);
    PySys_SetArgv((int)argv.size(), (char**)argv.data());

    //plugins path
    string path = sect->Get_string("path");
    if (path.empty()){
        Cross::CreatePlatformConfigDir(path);
        path += "python";
    }
    PyObject* sysPath = PySys_GetObject((char*)"path");
    PyObject* plpath = PyString_FromString(path.c_str());
    PyList_Append(sysPath, plpath);
    Py_DECREF(plpath);

    //init pydosbox
    init_dbox();
    init_break();
    dosboxUI = dbox_start() == 0;
    if (dosboxUI){
        DBGUI_StartUp();
        PyRun_SimpleString("from dosbox import *");
    }
}


#endif
