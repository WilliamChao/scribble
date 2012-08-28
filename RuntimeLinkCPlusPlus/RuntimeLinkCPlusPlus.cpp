// RuntimeLinkCPlusPlus.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include <windows.h>
#include <imagehlp.h>
#include <cstdio>
#include <vector>
#pragma comment(lib, "imagehlp.lib")
#pragma warning(disable: 4996) // _s ����Ȃ� CRT �֐��g���Ƃł���





template<size_t N>
inline int istsprintf(char (&buf)[N], const char *format, ...)
{
    va_list vl;
    va_start(vl, format);
    int r = _vsnprintf(buf, N, format, vl);
    va_end(vl);
    return r;
}

template<size_t N>
inline int istvsprintf(char (&buf)[N], const char *format, va_list vl)
{
    return _vsnprintf(buf, N, format, vl);
}

#define istPrint(...) DebugPrint(__FILE__, __LINE__, __VA_ARGS__)

static const int DPRINTF_MES_LENGTH  = 4096;
void DebugPrintV(const char* /*file*/, int /*line*/, const char* fmt, va_list vl)
{
    char buf[DPRINTF_MES_LENGTH];
    //istsprintf(buf, "%s:%d - ", file, line);
    //::OutputDebugStringA(buf);
    //WriteLogFile(buf);
    istvsprintf(buf, fmt, vl);
    ::OutputDebugStringA(buf);
}

void DebugPrint(const char* file, int line, const char* fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    DebugPrintV(file, line, fmt, vl);
    va_end(vl);
}

bool InitializeDebugSymbol(HANDLE proc=::GetCurrentProcess())
{
    if(!::SymInitialize(proc, NULL, TRUE)) {
        return false;
    }
    ::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

    return true;
}




bool MapFile(const char *path, std::vector<char> &data)
{
    if(FILE *f=fopen(path, "rb")) {
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        if(size > 0) {
            data.resize(size);
            fseek(f, 0, SEEK_SET);
            fread(&data[0], 1, size, f);
        }
        fclose(f);
        return true;
    }
    return false;
}


char * SzStorageClass1[] = {
    "NULL", "AUTOMATIC", "EXTERNAL", "STATIC", "REGISTER", "EXTERNAL_DEF", "LABEL",
    "UNDEFINED_LABEL", "MEMBER_OF_STRUCT", "ARGUMENT", "STRUCT_TAG",
    "MEMBER_OF_UNION", "UNION_TAG", "TYPE_DEFINITION", "UNDEFINED_STATIC",
    "ENUM_TAG", "MEMBER_OF_ENUM", "REGISTER_PARAM", "BIT_FIELD"
};
char * SzStorageClass2[] = {
    "BLOCK","FUNCTION","END_OF_STRUCT","FILE","SECTION","WEAK_EXTERNAL"
};
PSTR GetSZStorageClass(BYTE storageClass)
{
    if ( storageClass <= IMAGE_SYM_CLASS_BIT_FIELD )
        return SzStorageClass1[storageClass];
    else if ( (storageClass >= IMAGE_SYM_CLASS_BLOCK)
        && (storageClass <= IMAGE_SYM_CLASS_WEAK_EXTERNAL) )
        return SzStorageClass2[storageClass-IMAGE_SYM_CLASS_BLOCK];
    else
        return "???";
}

void GetSectionName(WORD section, PSTR buffer, unsigned cbBuffer)
{
    char tempbuffer[10];
    switch ( (SHORT)section )
    {
    case IMAGE_SYM_UNDEFINED:   strcpy(tempbuffer, "UNDEF"); break;
    case IMAGE_SYM_ABSOLUTE:    strcpy(tempbuffer, "ABS  "); break;
    case IMAGE_SYM_DEBUG:       strcpy(tempbuffer, "DEBUG"); break;
    default:                    sprintf(tempbuffer, "%-5X", section);
    }
    strncpy(buffer, tempbuffer, cbBuffer-1);
}

bool DynamicLink(void *objdata)
{
    size_t ImageBase = (size_t)objdata;
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)ImageBase;
    if( pDosHeader->e_magic!=IMAGE_FILE_MACHINE_I386 || pDosHeader->e_sp!=0 ) {
        return false;
    }

    PIMAGE_FILE_HEADER pImageHeader = (PIMAGE_FILE_HEADER)ImageBase;
    PIMAGE_OPTIONAL_HEADER *pOptionalHeader = (PIMAGE_OPTIONAL_HEADER*)(pImageHeader+1);

    PIMAGE_SYMBOL pSymbolTable = (PIMAGE_SYMBOL)((size_t)pImageHeader + pImageHeader->PointerToSymbolTable);
    DWORD SymbolCount = pImageHeader->NumberOfSymbols;

    for(size_t si=0; si<pImageHeader->NumberOfSections; ++si) {
        PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)(ImageBase + sizeof(IMAGE_FILE_HEADER) + pImageHeader->SizeOfOptionalHeader) + si;
        DWORD NumRelocations = pSectionHeader->NumberOfRelocations;
        PIMAGE_RELOCATION pRelocation = (PIMAGE_RELOCATION)(ImageBase + pSectionHeader->PointerToRelocations);
        for(size_t ri=0; ri<NumRelocations; ++ri) {
            PIMAGE_RELOCATION pReloc = pRelocation + ri;
            PIMAGE_SYMBOL pSym = pSymbolTable + pReloc->SymbolTableIndex;
            istPrint("");
        }
    }
    {

        istPrint(
            "Symbol Table - %X entries  (* = auxillary symbol)\n", SymbolCount);
        istPrint(
            "Indx Name                 Value    Section    cAux  Type    Storage\n"
            "---- -------------------- -------- ---------- ----- ------- --------\n");

        char SectionName[10];
        PSTR StringTable = (PSTR)&pSymbolTable[SymbolCount]; 
        for( size_t i=0; i < SymbolCount; ++i ) {
            istPrint("%04X ", i);
            if ( pSymbolTable->N.Name.Short != 0 ) {
                istPrint("%-20.8s", pSymbolTable->N.ShortName);
            }
            else {
                istPrint("%-20s", StringTable + pSymbolTable->N.Name.Long);
            }

            istPrint(" %08X", pSymbolTable->Value);

            GetSectionName(pSymbolTable->SectionNumber, SectionName, sizeof(SectionName));
            istPrint(" sect:%s aux:%X type:%02X st:%s\n",
                SectionName,
                pSymbolTable->NumberOfAuxSymbols,
                pSymbolTable->Type,
                GetSZStorageClass(pSymbolTable->StorageClass) );

            i += pSymbolTable->NumberOfAuxSymbols;
            pSymbolTable += pSymbolTable->NumberOfAuxSymbols;
            pSymbolTable++;
        }
    }
    return true;
}



void MakeExecutable(void *p, size_t size)
{
    DWORD old_flag;
    VirtualProtect(p, size, PAGE_EXECUTE_READWRITE, &old_flag);
}

// f: functor [](const char *symbol_name, const void *data)
template<class F>
void EachSymbol(void *objdata, const F &f)
{
    size_t ImageBase = (size_t)objdata;
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)ImageBase;
    if( pDosHeader->e_magic!=IMAGE_FILE_MACHINE_I386 || pDosHeader->e_sp!=0 ) {
        return;
    }

    PIMAGE_FILE_HEADER pImageHeader = (PIMAGE_FILE_HEADER)ImageBase;
    PIMAGE_OPTIONAL_HEADER *pOptionalHeader = (PIMAGE_OPTIONAL_HEADER*)(pImageHeader+1);

    PIMAGE_SYMBOL pSymbolTable = (PIMAGE_SYMBOL)((size_t)pImageHeader + pImageHeader->PointerToSymbolTable);
    DWORD SymbolCount = pImageHeader->NumberOfSymbols;

    PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)(ImageBase + sizeof(IMAGE_FILE_HEADER) + pImageHeader->SizeOfOptionalHeader);
    DWORD SectionCount = pImageHeader->NumberOfSections;

    PSTR StringTable = (PSTR)&pSymbolTable[SymbolCount]; 
    {
        for( size_t i=0; i < SymbolCount; ++i ) {

            IMAGE_SYMBOL &sym = pSymbolTable[i];
            if(sym.N.Name.Short == 0) {
                IMAGE_SECTION_HEADER &sect = pSectionHeader[sym.SectionNumber-1];
                const char *name = (const char*)(StringTable + sym.N.Name.Long);
                void *data = (void*)(ImageBase + sect.PointerToRawData);
                MakeExecutable(data, sect.SizeOfRawData);
                f(name, data);
            }
            i += pSymbolTable[i].NumberOfAuxSymbols;
        }
    }
}


typedef float (*FloatOpT)(float, float);
FloatOpT FloatAdd = NULL;
FloatOpT FloatSub = NULL;
FloatOpT FloatMul = NULL;
FloatOpT FloatDiv = NULL;


int main(int argc, _TCHAR* argv[])
{
    InitializeDebugSymbol();

    std::vector<char> obj;
    if(!MapFile("DynamicFunc.obj", obj)) {
        return 1;
    }
    EachSymbol(&obj[0], [](const char *name, const void *data){
        if(strcmp(name, "_FloatAdd")==0)        { FloatAdd = (FloatOpT)data; }
        else if(strcmp(name, "_FloatSub")==0)   { FloatSub = (FloatOpT)data; }
        else if(strcmp(name, "_FloatMul")==0)   { FloatMul = (FloatOpT)data; }
        else if(strcmp(name, "_FloatDiv")==0)   { FloatDiv = (FloatOpT)data; }
    });

    istPrint("%.2f\n", FloatAdd(1.0f, 2.0f));
    istPrint("%.2f\n", FloatSub(1.0f, 2.0f));
    istPrint("%.2f\n", FloatMul(1.0f, 2.0f));
    istPrint("%.2f\n", FloatDiv(1.0f, 2.0f));

    return 0;
}
