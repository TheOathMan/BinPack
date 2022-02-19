//
// BinPack
//
//
#if defined(_WIN32) 
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#include <iostream>
#define MINIZ_INCLUDE_SOURCE
#include "miniz.h"
#include <fstream>
#include <vector>
#include <string>
#include <functional>
//#include <stdio.h>
//#include <process.h>
//TODO:
// header strings for lib output and header output
// output two lib format (.a .lib)
// data need to be packed for lib output


void help(){
    const std::string help = R"( 
List files that need to be packed as data array. 
Use minus sign (-) to list options. available options are:

*output options
  -hr  to pack all file's data in a header (.h). [default]
  -l64 to pack all file's data into a 64 library file.
  -l32 to pack all file's data into a 32 library file.
  -p   output data to the console.
  -pn  output native data to the console.

*input options
  -bn to produce data array as binary literals.
  -hx to produce data array as hexadecimal literals.
  -j  to align data array or apply a (justify).
  -c  to compress the data before converting it to data array (using miniz.h)
        IMPORTANT: the size of the compressed file is stored as the first string. 
        so Compressed_data + strlen(Compressed_data) + 1 = the actual compressed Data that miniz can decompress.  

*Options are not casae sensetive. 
*Files and options can be listed at any order. 
)";
    std::cout << help << '\n'; 
    exit(EXIT_SUCCESS);
}

//  utility
//void (*clean)();
std::function<void()> clear;
#define ASSERT(condition, message) \
    do { \
        if (condition) { \
            std::cerr << message << '\n'; \
            exit(EXIT_FAILURE); \
        } \
    } while (false)

using uchar = unsigned char;
using uint =  unsigned int;
using ulong =  unsigned long;
using padSize_t = uint;

#define MAX_SIZE (~0u) 

#define _FREE(x)        ( free(x))
#define DR_2_C_P(x)     ((char*)(*x))
#define DR_2_UC_P(x)    ((uchar*)(*x))

#if  defined(__MINGW64__) || defined(__GNUC__)
#define ITOA(x,y,z) itoa(x,y,z)
#define STRWR(x) strlwr(x)
#define POPEN(x,y) popen(x,y)
#define PCLOSE(x) pclose(x)
#elif defined(_WIN32)
#define ITOA(x,y,z) _itoa(x,y,z)
#define STRWR(x) _strlwr(x)
#define POPEN(x,y) _popen(x,y)
#define PCLOSE(x) _pclose(x)
#endif




enum {arc_x86,arc_x64}; // libs architecture

enum opt_flags : int {
    op_none       = 0     ,
    //output options
    op_header     = 1 << 0,  // output data as an array in a header file (default)
    op_lib32      = 1 << 1,  // output data in a 32 library
    op_lib64      = 1 << 2,  // output data in a 64 library
    op_print      = 1 << 3,  // output data in te console
    op_printn     = 1 << 4,  // output data natively te console
    //input options
    op_bin        = 1 << 5,  // data array as binery
    op_hex        = 1 << 6,  // data array as hexadacimal
    op_compressed = 1 << 7,  // compress option (using miniz.h)
    op_justify    = 1 << 8,  // align data array or apply (justify)
};

struct FileWrite{
    const char* name;
    std::ofstream os;
    bool flushed=false;
    void open(const char* name,std::ios_base::openmode mode ){ 
        this->name = name; os.open(name,mode);  ASSERT(!os.good(), "Error while creating: " <<"'" << name << "'");}
    void write(const char* dat,size_t s ){ os.write(dat,s); }
    void clear(){ if(!flushed) {os.close(); std::remove(name);} }
    void close(){ flushed=true; os.flush(); os.close();}
    std::ofstream& operator()(){ return os; }
    ~FileWrite(){clear();}
}hrwrite, libWrite;

namespace LibGen{
    //refer to data in lib
    //extern "C" char const*  __LIB_DATA__();

    // compressed .lib template binary (arc_x64) 
    const static unsigned char com_lib64_template_dat[]{
        0x78,0x01,0x75,0x52,0xb1,0x4e,0xc3,0x30,0x10,0xbd,0xd0,0x82,0xaa,0x0e,0x50,0xb1,
        0x31,0x61,0x7e,0xc0,0x8d,0x4a,0xe8,0x84,0x50,0x82,0x18,0x40,0x62,0xa8,0x20,0x13,
        0x8b,0x6b,0x9a,0xb4,0x54,0x0a,0xa2,0x4a,0x2c,0xc4,0xc8,0xc4,0xc6,0x3f,0x55,0xe2,
        0x13,0x58,0x18,0x18,0x99,0xd9,0x19,0x4a,0x7b,0xe7,0xa6,0x8d,0xdd,0xb4,0x4f,0x3a,
        0xfb,0x7c,0x7e,0x79,0xbe,0xe7,0xf8,0xe8,0x54,0xa6,0xbd,0x87,0xb3,0x7a,0x93,0xd9,
        0x70,0x4b,0xb9,0x39,0x32,0xd6,0x6a,0x2d,0x76,0xbb,0x75,0x00,0x70,0x30,0xee,0x84,
        0xb8,0xbe,0x3a,0x17,0x17,0x41,0x18,0x08,0x01,0xa0,0xe2,0x4c,0xf1,0xa7,0x42,0x77,
        0x93,0x62,0xdb,0xf3,0xf4,0x7c,0x72,0xdc,0x5e,0x2a,0x46,0x6f,0xdb,0x28,0x08,0x30,
        0xc1,0xd8,0xd5,0x19,0x00,0x57,0xf1,0x8b,0xca,0x73,0x42,0x0d,0xe3,0x1b,0xe3,0x27,
        0x5f,0x53,0x0b,0x0c,0x3a,0x5d,0x1e,0x49,0x25,0x0d,0x1e,0xe1,0x77,0x65,0xed,0x83,
        0x3b,0xe6,0xf7,0x59,0xb6,0x52,0x2e,0xe1,0x95,0x78,0xa9,0x2d,0xb8,0xb7,0x56,0xaf,
        0xe1,0x37,0xbd,0xf2,0xf7,0xd4,0xbf,0x85,0x5a,0x03,0x2e,0xdf,0xb5,0xb7,0x8f,0x0a,
        0x0e,0x3b,0x18,0x55,0x10,0xf3,0x3b,0xbb,0x0d,0x83,0x9b,0x10,0x6f,0xce,0x34,0x4a,
        0xa6,0x2a,0x0e,0x59,0xa5,0xec,0xb3,0x73,0x10,0xd0,0x8c,0x97,0x61,0xf4,0xb4,0xa5,
        0x39,0xe6,0x29,0x54,0x41,0x8e,0x61,0x90,0x0e,0xb3,0x39,0x54,0x41,0x8e,0x61,0xae,
        0xaa,0x39,0x64,0x8f,0xf0,0xf7,0x35,0x9e,0x50,0xa5,0xd8,0x23,0x50,0xeb,0xb6,0xce,
        0xfc,0x47,0xf9,0xfd,0x58,0x2a,0xee,0xba,0x94,0x4f,0xa7,0x0b,0xf5,0xfd,0xa5,0x0b,
        0x86,0x3d,0xf1,0xfe,0x30,0x89,0xf3,0xca,0x3f,0x72,0x06,0x8e,0x7e,0x22,0xbd,0xd1,
        0xa8,0x50,0x3b,0xa4,0x9e,0x92,0xe4,0xf9,0x51,0xc8,0x28,0x4a,0xb3,0xe1,0x00,0x2f,
        0xc7,0x78,0x54,0x33,0x36,0xc4,0x53,0x25
    };
    const static unsigned char com_lib32_template_dat[]{
                            0x78,0x01,0x75,0x52,0x31,0x4b,0xc3,0x40,0x14,0xbe,0x58,0x15,
        0x89,0xa0,0xc5,0x5f,0x70,0x2e,0x8e,0xd7,0x50,0xa2,0x93,0x48,0x22,0x2e,0x42,0x07,
        0xd1,0xb8,0x08,0x72,0x39,0x9b,0xb4,0x16,0x22,0x96,0xe4,0x10,0x47,0x57,0x07,0x77,
        0x7f,0x89,0x73,0xc1,0x3f,0xe1,0xe0,0x2a,0xce,0x82,0x8b,0x83,0x6d,0xdf,0xbb,0x4b,
        0x9a,0x4b,0xd3,0x7e,0xf0,0x72,0xef,0xde,0x7d,0x7c,0x79,0xdf,0xbb,0xdb,0x3d,0x14,
        0x69,0xf7,0xf6,0xc8,0x6e,0xd1,0x2a,0x9c,0x5a,0x6e,0x7e,0x29,0x6d,0xb7,0x8b,0xd3,
        0xd0,0x26,0x84,0x58,0x10,0x57,0x9c,0xf3,0xce,0xe9,0x31,0x3f,0xf1,0x03,0x9f,0x73,
        0x22,0xe3,0x4c,0xb2,0xfb,0x52,0x77,0x99,0xe2,0x81,0xeb,0xaa,0x75,0xdf,0x2d,0x18,
        0xa1,0xdd,0xb1,0xd6,0x40,0x50,0xeb,0x6e,0xe9,0x8c,0x30,0x19,0x3f,0xca,0x3c,0x47,
        0x6c,0x42,0x7c,0x42,0x7c,0xe7,0x7b,0x6c,0x81,0x92,0xb3,0x90,0x45,0x42,0x0a,0x83,
        0x87,0xf8,0x99,0xdb,0x7b,0xc4,0x19,0xb1,0x9b,0x2c,0x9b,0x2b,0xd7,0xf0,0x84,0xbc,
        0xb4,0x2a,0xb8,0xbd,0x50,0xaf,0xe9,0xb5,0xdc,0x05,0x02,0xd8,0x94,0x89,0x8d,0x26,
        0xb9,0x7c,0xfe,0x7a,0x51,0xee,0xae,0xdf,0x71,0x59,0x57,0xc1,0xf5,0xd4,0x2e,0x02,
        0xff,0x3c,0x80,0xd9,0x99,0x5e,0x51,0xa2,0x61,0xa1,0x5b,0xcc,0xde,0x5e,0xf7,0x7e,
        0xb5,0xa8,0xe9,0x73,0x45,0x71,0xcc,0x1f,0x61,0x05,0x38,0x86,0xc7,0x46,0x8d,0x83,
        0x15,0xe0,0x18,0xfe,0x56,0x15,0x07,0x1d,0x22,0xfe,0x3e,0x46,0xff,0x58,0x29,0xcf,
        0x10,0xd8,0x74,0x55,0x47,0xdf,0x95,0xd7,0x8b,0x85,0x64,0x8e,0x83,0x47,0x93,0x49,
        0xa1,0xbe,0x33,0x73,0x41,0xa1,0x27,0xd6,0x1b,0x24,0x71,0x5e,0x19,0x03,0xa7,0x6f,
        0xa9,0x57,0xd2,0x1d,0x0e,0x4b,0x35,0x8a,0x3d,0x25,0xc9,0xc3,0x1d,0x17,0x51,0x94,
        0x66,0x83,0x3e,0x0c,0xc7,0x7c,0x57,0x53,0x8f,0x07,0x54,0x21
    };
    int _arc_offset = 0x3;
    int _od_size = 0x0f;                                 //initial size
    #define lib64_original_size 686
    #define lib32_original_size 690
    #define S_SIZE_VAL    (521 + LibGen::_od_size)             // string size
    #define S_SIZE_VAL_32 (525 + LibGen::_od_size)             // string size
    #define SYMPOL_VAL    (238 + LibGen::_arc_offset + LibGen::_od_size)

    #define TO_DATA (0x184 + LibGen::_arc_offset)
    #define TO_SIZE_1 0x132                  // value = _od_size
    #define TO_SIZE_2    ( 0x202 + LibGen::_arc_offset + LibGen::_od_size )   // value = _od_size
    #define TO_S_SIZE 0x8A                   // string size
    #define TO_SYMPOL_P1 0x9E                //pointer to data address1
    #define TO_SYMPOL_P2 0x15E               //pointer to data address2

    #define IN_VAL_32(x,y)  *((uint*)(x + y))
    struct LibSecs{uchar* p1,*p2; size_t s1,s2; void freep(){_FREE(p1);} };

    LibSecs GetLibSec(bool arc = arc_x64){
        ASSERT(arc>1, "Wrong architecture value");
        int or_size = arc == arc_x64  ? lib64_original_size : lib32_original_size;
        uchar* uc_lib_templ_data = (uchar*)malloc(or_size);
        ASSERT(!uc_lib_templ_data, "Failed to create a library. allocation error!" );
        ulong fsize = or_size;
        if(arc == arc_x64) { 
            _arc_offset = 0;
            mz_uncompress(uc_lib_templ_data, &fsize, com_lib64_template_dat, lib64_original_size);
        } else 
        if(arc == arc_x86)
            mz_uncompress(uc_lib_templ_data, &fsize, com_lib32_template_dat, lib32_original_size);
        LibSecs secs;
        secs.p1 = uc_lib_templ_data;
        secs.s1 = TO_DATA;
        secs.p2 = uc_lib_templ_data + TO_DATA + _od_size;
        secs.s2 = or_size - TO_DATA - _od_size;

        return secs;
    }
}

void puts(std::ofstream& of,char c, int count){ for(int i =0; i < count; i++){of.put(c);} of.put('\n');  }

static char str_buffer[4*4];
   
const char* char2Hex(uchar c, int opflags) {
    memset(str_buffer,0,sizeof(str_buffer));
    char tempbuf[5]; 
    ITOA(c,tempbuf,16);
    if(!(opflags & op_printn))
        strcat(str_buffer,"0x");
    if((opflags & op_justify) && (strlen(tempbuf) == 1)) 
        strcat(str_buffer,"0");
    strcat(str_buffer,tempbuf);
    return str_buffer;
}
const char* char2bin(uchar c,int opflags) {
    char tempbuf[12]; 
    memset(str_buffer,0,sizeof(str_buffer));
    const char* zez = "00000000";
    ITOA(c,tempbuf,2);
    const char* p; 
    p = zez + (strlen(tempbuf)); 
    if(!(opflags & op_printn))
        strcat(str_buffer,"0b");
    if(opflags & op_justify)
        strcat(str_buffer,p);
    strcat(str_buffer,tempbuf);
    return str_buffer;
}

const char* char2dec(uchar c,int opflags){ 
    //ASSERT_IS
    memset(str_buffer,0,sizeof(str_buffer));
    const char* s="   ";
    char tempbuf[4];
    ITOA(c,tempbuf,10);

    if(opflags & op_justify) 
        strcat(str_buffer,s + strlen(tempbuf));  
    strcat(str_buffer,tempbuf);
    
    return str_buffer; 
}


// functions
void OpenReadBin(const char* file_name, void** outData, size_t* outSize){
    std::ifstream st;
    st.open(file_name,std::ios::in | std::ios::binary);
    ASSERT(!st.good(), "can't open" <<"'" << file_name << "'");
    st.seekg(0,st.end);
    int s = st.tellg();
    ASSERT(!s, "file" << "'" << file_name << "'" << " is empty");
    st.seekg(st.beg);
    *outData = malloc(s);
    st.read((char*)*outData,s);
    *outSize = s;
    auto a = 0xef;
    st.close(); 
}



int CompressOut(padSize_t size, void* data, void **comDataOut, ulong* comSizeOut ){ 
    ASSERT(!size, "No initial size!");
    *comSizeOut = mz_compressBound(size);
    std::string s_size(std::to_string(size));
    *comDataOut = malloc(*comSizeOut + s_size.length()+1);
    strcpy(DR_2_C_P(comDataOut),s_size.c_str());
    int e = mz_compress(DR_2_UC_P(comDataOut) + s_size.length()+1 ,comSizeOut,(uchar*)data,size);
    *comSizeOut += s_size.length()+1;
    ASSERT(e,"Compression Error");
    return e;
}


int DecompressOut(void* Comdata, void **ucmDataOut, ulong* uncmSizeOut ){ 
    padSize_t fullSize = 0;
    sscanf((char*)Comdata, "%d", &fullSize);
    *uncmSizeOut = fullSize;
    *ucmDataOut = (uchar*)malloc(fullSize);
    int e = mz_uncompress(DR_2_UC_P(ucmDataOut), uncmSizeOut, ((uchar*)Comdata) + strlen((char*)Comdata) + 1, fullSize);
    ASSERT(e,"Decompression Error");
    return e;  
}


const char* GetFileNameExclude_(const char* str, char extra = '/'){
    const char* p;
    for(p = str + strlen(str);  p >= str && *p != extra && *p != '/' && *p != '\\'; p-- ){}
    return p+1;
}

std::string FPathToName(std::string path){
    std::string sn( GetFileNameExclude_(path.c_str()));
    sn.erase(sn.find('.'));
    return sn;
}

//const unsigned char name[100] = {}; //
template <typename T> 
void PackData(T& ofs, void* data, const padSize_t& size, std::string&& name,int op_flags){
    
    static int PrevSize=0,calls=0;
    if(op_flags & (op_lib64 | op_lib32) ){
        if(!PrevSize)
            ofs << R"(extern "C" char const*  __LIB_DATA__();)" << "\n\n"; 
        ofs << "#define "<< name << "_size" << " " << size <<"\n"; 
        
        ofs << "#define "<< "_"<< name << "_data" << " &(__LIB_DATA__()[" << PrevSize <<"])" <<"\n\n"; 
        PrevSize = size;
        //LibGen::DataToLib(data,size,op_flags & op_lib64 ? arc_x64 : arc_x86); 
        return;
    }
    
    bool is_hex     = op_flags & op_hex;
    bool is_bin     = op_flags & op_bin;
    bool is_justify = op_flags & op_justify;

    if(!(op_flags & (op_print | op_printn ) )){    
        auto pos = ofs.tellp();
        ofs.seekp(ofs.beg + calls*101 ); 
        ofs << "const unsigned char CM_"<< name << "[" << std::to_string(size) << "];";
        ofs.seekp(pos);

        ofs << "const unsigned char CM_" << name << "[" << std::to_string(size) << "] " << "= { \n";
    
    }
    for (size_t i = 0; i < size; i++)
    {
        int value = ((uchar*)data)[i];
        if(is_hex) ofs << char2Hex(value,op_flags); else
        if(is_bin) ofs << char2bin(value,op_flags); else 
                   ofs << char2dec(value,op_flags); 

        if(i < size-1)
            ofs << (!(op_flags & op_printn) ? "," : " ") << (( ((i+1) % (is_hex ? 8*2 : is_bin ? 10 : 30)) == 0) ? "\n" : "" );
        else if(!(op_flags & (op_print | op_printn ))) ofs << "}; \n\n";
        else ofs << '\n';
    }calls++;
}

// Ignore previous conflicting options 
void IgnoreFlags(const char* s,int& opflag, int conflicting_flags)
{
    auto put = [&](const char* c,int f){    
        if(conflicting_flags & f && f & opflag){
            std::cout << "Optoin -" << c <<" was ignored because it conflicts with option "<< "'" << "-"<< s << "'" <<  '\n';
            opflag = opflag ^ f;
        }
    };
    put("l32",op_lib32); put("l64",op_lib64);   put("p",op_print);     put("pn",op_printn);
    put("j",op_justify); put("bn",op_bin);      put("hx",op_hex);      put("hr",op_header); 
}



int main(int count, const char* args[]){

    //std::cout << exec("g++ -help") << std::endl; 
    if(count == 1) help();
    
    int opFlags = op_header;
    std::vector<std::string> files;
    std::vector<std::string> options;
    std::string outputFile("Resources.h");
    //FileWrite tempf;
    //tempf.open("tssf",0);
    //libWrite.exceptions(std::ios::badbit | std::ios::failbit);
    //clear = [&](){hrwrite.close();libWrite.close(); std::remove("Resources.h");std::remove("libdata64.a");std::remove("libdata32.a");}; //FIXME:
    LibGen::LibSecs spec;
    size_t totalfsize=0;


    for (size_t i = 1; i < count; i++)
    {
        if(args[i][0] != '-') {  files.push_back(args[i]  ); }
        else                  {  options.push_back(args[i]); }
    }
    for(auto && i: options){
        STRWR(&i[0]);   
        #define add(this_flag,option,ignore_flags) if(strstr(i.c_str(),option))   {IgnoreFlags(option,opFlags,ignore_flags ); opFlags |= this_flag;continue;} 
        add(op_header,    "hr"  ,op_lib32 | op_lib64 | op_print   | op_printn);
        add(op_hex,       "hx"  ,op_bin   | op_lib32 | op_lib64);
        add(op_bin,       "bn"  ,op_hex   | op_lib32 | op_lib64);
        add(op_justify,   "j"   ,op_lib32 | op_lib64);
        add(op_compressed,"c",0 );
        add(op_lib32,     "l32" ,op_print | op_printn | op_justify | op_hex  | op_bin | op_lib64 | op_header);
        add(op_lib64,     "l64" ,op_print | op_printn | op_justify | op_hex  | op_bin | op_lib32 | op_header);
        add(op_printn,    "pn"  ,op_print | op_lib32  | op_lib64   | op_header);
        add(op_print,     "p"   ,op_lib32 | op_lib64  | op_printn  | op_header);
        ASSERT(true,"option '" << i <<"' doesn't exist" );  
    }

    //open header file stream
    if(!(opFlags & (op_print | op_printn)))
        hrwrite.open(outputFile.c_str(),std::ios::out | std::ios::trunc);

    //open lib file stream
    if(opFlags & (op_lib64 | op_lib32)){
        libWrite.open(opFlags & op_lib64 ? "libdata64.a" : "libdata32.a",std::ios::out | std::ios::binary);
        spec = LibGen::GetLibSec(opFlags & op_lib64 ? arc_x64:arc_x86);
        libWrite.write((char*)spec.p1,spec.s1);
    }

    for (auto&& i: files){

        void* data=nullptr;
        size_t size=0;
        OpenReadBin(i.c_str(),&data,&size);
        ASSERT(size >= MAX_SIZE, "file: " << i.c_str()  << " size is too big" );
        ASSERT(!data,"no data in: " << i.c_str());


        //start header file by reserve/define
        if(!totalfsize && (opFlags & op_header)) {
            for(auto&& n: files) puts(hrwrite(),' ', 100);
            puts(hrwrite(),' ', 100); puts(hrwrite(),' ', 100);
            hrwrite() << "#ifdef INSER_RESOURCES\n";
        }

        totalfsize+=size;
        if(opFlags & op_compressed){
            mz_ulong csized = 0;
            void* c_data = nullptr;
            CompressOut(size,data,&c_data,&csized);      
            printf("\nsize of '%s' file is: %i bytes. compressed size: %i bytes\n\n", i.c_str(), size,csized);
            
            PackData(opFlags & (op_print | op_printn ) ? std::cout : hrwrite(), c_data, csized, FPathToName(i), opFlags);
            if(opFlags & (op_lib64 | op_lib32))
                libWrite.write((char*)c_data,csized);

            _FREE(c_data);
        } else {
            printf("\nsize of '%s' file is: %i bytes \n\n", i.c_str(), size);
            PackData(opFlags &  (op_print | op_printn ) ? std::cout : hrwrite(), data, size, FPathToName(i), opFlags);
            if(opFlags & (op_lib64 | op_lib32))
                libWrite.write((char*)data,size);
        }

        _FREE(data);
    }
    
    // close header
    if(opFlags & op_header) 
        hrwrite() << "\n#endif // INSER_RESOURCES";
    

    //libgen close();
    if(opFlags & (op_lib64 | op_lib32)){
        libWrite.write((char*)spec.p2,spec.s2);
        #define F_PTR(seek,d,size) libWrite().seekp(libWrite().beg + seek); libWrite().write(((char*)(d)),size );
        LibGen::_od_size = totalfsize;
        size_t symv = SYMPOL_VAL;
        F_PTR(TO_SIZE_1,&LibGen::_od_size,sizeof(uint));
        F_PTR(TO_SIZE_2,&LibGen::_od_size,sizeof(uint));
        F_PTR(TO_SYMPOL_P1,&symv,sizeof(uint));
        F_PTR(TO_SYMPOL_P2,&symv,sizeof(uint));
        char buf[10];
        ITOA(opFlags & op_lib64 ? S_SIZE_VAL:S_SIZE_VAL_32,buf,10); 
        F_PTR(TO_S_SIZE,buf,strlen(buf));
        //libWrite.seekp(libWrite.end);
        libWrite.close();
        spec.freep();
    }

    hrwrite.close();
    puts("Success");
    return 0;
}