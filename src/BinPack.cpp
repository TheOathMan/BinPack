/* BinPack.cpp v 1.101 - public domain. Write data of files into a header file, static lib or as a console output.
*/
#if defined(_WIN32) 
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#include <iostream>
#define MINIZ_INCLUDE_SOURCE
#include "miniz.h"
#include <fstream>
#include <vector>
#include <string>

void howto(){
    const std::string help = R"( 
List files that need to be packed as data array in a single header file or as a static libray.
Use minus sign (-) to list options, and equal sign (=) for edit options. Available options are:

--Write Options:
  -hr   To pack all the data of provided files into a header file (.h). [default]
  -l64  To pack all the data of provided files into a 64 bit static library.
  -l32  To pack all the data of provided files into a 32 bit static library.
  -p    Output data to the console.
  -pn   Output data to the console natively. (strip c++ integer-suffix and comma)
  -pc   Output data to the console as Ascll characters.

--Edit Options:
  =out  <FOLDER> write the output to a folder. Example: =out C:\Users\Desktop 
  =jl   <NUMBER> Edit the justify level or end line level. Example: =jl 2 
  =cmf  <FILE> compress a file.
  =ucf  <FILE> decompress a file.

--Mode Options:
  -hx   Convert data to hexadecimal literals.
  -bn   Convert data to binary literals.
  -j    To align the data or justify all lines.
  -c    To compress the data. (see sorce code for more details on how to decompress)

*Options are not casae sensetive. 
*Files and options can be listed at any order. 
*Edit Options and their values must be seperated by a space as in [EDIT OPTION] [VALUE].    

EXAMPLE1:
> text.txt -j -hx -p
This command will prin the data of 'text.txt' into the console(-p) in the form of hexadecimal(-hx),
and justify(-j) will be applied to the test.

EXAMPLE2:
> text.txt data.bin -l64
This command will pack the binary data of 'text.txt' and 'data.bin' into a static library file (.lib/.a) 
which then will be outputted along with a header file that contain pointers to each data within that precompiled binary package.


)";
    std::cout << help << '\n'; 
    exit(EXIT_SUCCESS);
}


#define ASSERT(condition, message) \
    do { \
        if (condition) { \
            std::cerr << "[ERROR]: " << message << '\n'; \
            exit(EXIT_FAILURE); \
        } \
    } while (false)

using uchar = unsigned char;
using uint =  unsigned int;
using ulong =  unsigned long;
using padSize_t = uint;
using RSTR = const char*;

#define MAX_SIZE (~0u) 
#define LINEPAD_SIZE 100 

#define _FREE(x)        ( free(x))
#define DR_2_C_P(x)     ((char*)(*x))
#define DR_2_UC_P(x)    ((uchar*)(*x))
//#define NO_FILE_OUT if(files.empty())exit(EXIT_SUCCESS)
#define BIN_WRITE std::ios::out | std::ios::binary
#define TXT_WRITE std::ios::out | std::ios::trunc
#define BIN_READ  std::ios::in | std::ios::binary


#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEPARATOR '/'
#elif defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS 1
#define PATH_SEPARATOR '\\'
#endif


#if defined(__APPLE__) || defined(__linux__)
#include <cstring>
#include <cstdlib>
#define ITOA(value, buff, rx) snprintf(buff, sizeof(buff), "%d", value)
#define STRWR(value) std::transform(value, value + strlen(value), value, ::tolower)
#define POPEN(value, buff) popen(value, buff)
#define PCLOSE(value) pclose(value)
#elif  defined(__MINGW64__) || defined(__GNUC__)
#define ITOA(value,buff,rx) itoa(value,buff,rx)
// #define ITOA(value, buff, rx) snprintf(buff, sizeof(buff), "%d", value)
#define STRWR(value) strlwr(value)
#define POPEN(value,buff) popen(value,buff)
#define PCLOSE(value) pclose(value)
#elif defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS 1
#define ITOA(value, buff, rx) _itoa(value, buff, rx)
#define STRWR(value) _strlwr(value)
#define POPEN(value, buff) _popen(value, buff)
#define PCLOSE(value) _pclose(value)
#endif


enum {arc_x86,arc_x64}; // libs architecture

enum opt_flags : uint {
    op_none       = 0     ,

    op_header     = 1 << 0,  // write data as an array into a header file (default).
    op_lib32      = 1 << 1,  // write data into a 32 library.
    op_lib64      = 1 << 2,  // write data into a 64 library.
    op_print      = 1 << 3,  // write data into the console.
    op_printn     = 1 << 4,  // write data natively  into the console.
    op_printc     = 1 << 5,  // write data natively  into the console.
    op_libs       = op_lib64  | op_lib32,
    op_prints     = op_printn | op_print | op_printc ,
    op_writeops   = op_prints | op_lib64 | op_lib32 | op_header,
    
    op_bin        = 1 << 6,  // data array as binery.
    op_hex        = 1 << 7,  // data array as hexadacimal.
    op_compressed = 1 << 8,  // compress option (using miniz.h).
    op_justify    = 1 << 9,  // align data array or apply (justify).
    op_mods       = op_bin | op_hex | op_justify,

    op_out        = 1 << 10,  // output path to write to.
    op_jl         = 1 << 11, 
    op_fcompress  = 1 << 12, 
    op_funcompress = 1 << 13  
};

struct ComProc{
    void* m_data;
    ulong m_size;
    ComProc(){}
    ComProc(padSize_t dsize, void* data, std::string name = " "){
        ASSERT(!dsize, "No initial size!");
        m_size = mz_compressBound(dsize);
        std::string s_size(std::to_string(dsize));
        size_t extra_size =  s_size.length()+1 + name.length()+1;
        m_data = malloc( m_size+extra_size);
        strcpy((char*)(m_data),s_size.c_str());
        strcpy((char*)(m_data) + s_size.length() + 1,name.c_str());

        int e = mz_compress((uchar*)(m_data) + extra_size ,&m_size,(uchar*)data,dsize);
        m_size += extra_size;
        ASSERT(e || dsize < 20 ,"Coudn't compress this file. Size might be too small. ERROR CODE: " << e);

    }
    void freedat(){_FREE(m_data);} 
};

struct DcomProc{ 
    void* m_data=nullptr; 
    mz_ulong m_size=0; 
    RSTR name; 
    DcomProc(void* Comdata){
        sscanf((char*)Comdata, "%d", &m_size);
        name = (char*)Comdata + strlen((char*)Comdata) +1;
        int to_data = strlen(name) + strlen((char*)Comdata) + 2;
        padSize_t fullsize = m_size;
        m_data = (uchar*)malloc(m_size);
        int e = mz_uncompress((uchar*)(m_data), &m_size, (uchar*)Comdata + to_data , fullsize);
        ASSERT(e,"Coudn't decompress this file. Wrong format or corrupted file. ERROR CODE: " << e);
    }
    void freedat(){_FREE(m_data);} 
};

struct FileWrite{
    const char* path;
    std::ofstream os;
    size_t m_size;
    bool flushed=false;
    void * m_data;
    std::ios_base::openmode Mode;
    FileWrite(){};
    FileWrite(const char* name,std::ios_base::openmode mode){init(name,mode);};
    void open(const char* name,std::ios_base::openmode mode ){ init(name,mode);}
    void write(const char* dat,size_t s ){ os.write(dat,s); m_data = (void*)dat; m_size=s; }
    void clear(){ if(os.is_open()) os.close(); if(!flushed) {  if(path) printf("cleaning unflushed resources '%s'\n",path); std::remove(path);} }
    void close( void(*OnClean)() = nullptr ){ flushed=true;  os.flush(); os.close();  if(OnClean) OnClean();}
    void writefb( std::filebuf* fb){ os << fb;flushed=true; }
    std::ofstream& operator()(){ return os; }
    ~FileWrite(){clear();}
    private:
    void init(const char* name,std::ios_base::openmode mode){
        this->path = name; 
        os.open(name,mode);  
        ASSERT(!os.good(), "Coudn't create: " <<"'" << name << "'. Path may not exist!. ECODE: " << os.rdstate() );
    }
}hrwrite, libWrite;

struct FileRead{
    void* m_data;
    size_t m_size;
    RSTR m_fnmae;
    std::ifstream st;
    std::ios_base::openmode m_mode;
    FileRead() : m_data(nullptr) {}
    FileRead& operator=(const FileRead& rhs) { m_fnmae=rhs.m_fnmae; m_mode = rhs.m_mode;  init(m_fnmae,m_mode,false); return *this;  }
    FileRead(RSTR fname,std::ios_base::openmode mode,bool readonly=false): m_data(nullptr), m_fnmae(fname), m_mode(mode) {init(m_fnmae,mode,readonly);}
    void init(RSTR fname,std::ios_base::openmode mode,bool readonly){
        if(st.is_open()) st.close();
        st.open(fname,mode);
        ASSERT(!st.good(), "Coudn't open " <<"'" << fname << "'");
        st.seekg(0,st.end);
        int s = st.tellg();
        ASSERT(!s, "File" << "'" << fname << "'" << " is empty");
        st.seekg(st.beg);
        if(!readonly){
            m_data = malloc(s);
            st.read((char*)m_data,s);
            st.seekg(st.beg);
        }
        m_size = s;
    }
    std::ifstream& operator()(){ return st; }
    ~FileRead() { if(st.is_open()) st.close(); }
    void freedat(){ if(m_data) _FREE(m_data); }
}frd,tocom,toucm;




namespace LibGen{
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

void path2_c_fmt(std::string& input) {
    // Define the set of illegal characters as a string
    const std::string illegalChars = "/\\.*:?\"<>| ";

    // Iterate over each character in the input string
    for (size_t i = 0; i < input.size(); ++i) {
        // If the character is in the illegalChars string, replace it with '_'
        if (illegalChars.find(input[i]) != std::string::npos) {
            input[i] = '_';
        }
    }
}

void puts(std::ofstream& of,char c, int count){ for(int i =0; i < count; i++){of.put(c);} of.put('\n');  }
bool _striseq(RSTR s1,RSTR s2){ for(;*s1;) if(tolower(*s1++)!=tolower(*s2++)) return false;  return *s1 == *s2;  }
int striseq(RSTR s1,RSTR s2){ for(;*s1;) if((*s1++|32)!=(*s2++|32)) return 0; return *s1 == *s2;} // compre strings not case senstive
int strrep(char* t,char cr,char rp) { int rep=0; for(;strchr(t,cr);) {++rep; char* p = strchr(t,cr); *p = rp;} return rep;  } //replace char with char in a string
RSTR strffn(RSTR str, char extra = PATH_SEPARATOR){ RSTR p; for(p = str + strlen(str);  p >= str && *p != extra && *p != PATH_SEPARATOR; p-- ){}return p+1;} // get file name from path with file extention
RSTR strfn(char* fn){char* ps = strrchr(fn,'.'); *ps = '\0'; return strffn(fn);} // get file name from path
std::string& strfn(RSTR fn){ static std::string sn; sn.clear(); sn = std::string(strffn(fn) ); if(sn.find('.')!= std::string::npos) sn.erase(sn.find_last_of('.')); return sn;} // get file name from path

void copy_file(const std::string& source,const std::string& target)
{
    FileRead  fr(source.c_str(), BIN_READ);
    FileWrite fw(target.c_str(), BIN_WRITE);
    fw.writefb(fr().rdbuf());
}

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

template <typename T> 
void PackData(T& ofs, void* data, const padSize_t& size, std::string& name,int op_flags,int jv=0){
    ASSERT(name.length()>65, "Upnormal file name length" ); //? limit the number of chars in name
    path2_c_fmt(name);
    // removeIllegalChars(name.data());
    static int PrevSize=0;
    if(op_flags & op_libs ){
        if(!PrevSize)
            ofs << R"(extern "C" char const*  __LIB_DATA__();)" << "\n\n"; 
        ofs << "#define "<< name << "_size" << " " << size <<"\n";    
        ofs << "#define "<< "_"<< name << "_data" << " &(__LIB_DATA__()[" << PrevSize <<"])" <<"\n\n"; 
        PrevSize += size;
        libWrite.write((char*)data,size);
        return;
    }
    
    bool is_ascll   = op_flags & op_printc;
    bool is_hex     = op_flags & op_hex;
    bool is_bin     = op_flags & op_bin;
    bool is_justify = op_flags & op_justify;

    if(!(op_flags & op_prints ))  
        ofs << "const unsigned char BP_" << name << "[" << std::to_string(size) << "] " << "= { \n";
    
    for (size_t i = 0; i < size; i++)
    {

         
        int value = ((uchar*)data)[i];
        if(is_ascll){ofs << (isprint(value) ? (char)value : ' '); op_flags |= op_printn;   }else
        if(is_hex)   ofs << char2Hex(value,op_flags); else
        if(is_bin)   ofs << char2bin(value,op_flags); else 
                     ofs << char2dec(value,op_flags); 

        if(i < size-1)
            ofs << (!(op_flags & op_printn) ? "," : " ") << (( ((i+1) % (is_hex ? 8*2+jv : is_bin ? 10+jv : 30+jv)) == 0) ? "\n" : "" );
        else if(!(op_flags & op_prints )) ofs << "}; \n\n";
        else ofs << std::endl;
    }
}

// Ignore previous conflicting options 
void IgnoreFlags(RSTR s,int& opflag, int conflicting_flags)
{
    auto xclude = [&](RSTR c,int f){    
        if(conflicting_flags & f && f & opflag){
            std::cout << "Optoin '" << c <<"' was ignored because it conflicts with option '" << s << "'\n";
            opflag = opflag ^ f;
        }
    };
    xclude("l32",op_lib32); xclude("l64",op_lib64); xclude("p",op_print); xclude("pn",op_printn); xclude("out",op_out);
    xclude("j",op_justify); xclude("bn",op_bin);    xclude("hx",op_hex);  xclude("hr",op_header); xclude("pc",op_printc);
}

int opFlags = 0;
using StrPair=std::pair<std::string,std::string>;
std::vector<RSTR> files;
struct STRVAL{RSTR cmd,val;};
std::vector<STRVAL> options;
LibGen::LibSecs spec;
size_t totalfsize=0;
std::string outputpath;
int justifyval=0;

void OnLibWriteClose(){ 
    if(opFlags&op_lib32) 
        copy_file(outputpath+"libdata32.a",outputpath+"data32.lib");
    if(opFlags&op_lib64) 
        copy_file(outputpath+"libdata64.a",outputpath+"data64.lib");
}

int main(int count, const char* args[]){
    // char filename[] = "inva\\lid/fi*le:name?.txt";
    // removeIllegalChars(filename);
    // printf("%s\n",filename);


    if(count == 1) howto();
    
    for (size_t i = 1; i < count; i++)
    {
        if(args[i][0] == '-') options.push_back( {args[i],""}); else
        if(args[i][0] == '='){
            ///static bool pushDir;
            ASSERT(i+1 == count,"Empty edit field");
            RSTR p1=args[i],p2=args[++i];
            if(striseq(p1, "=out")) 
                options.insert(options.begin(), {p1,p2}); 
            else 
                options.push_back({p1,p2}); 
        } 
        else files.push_back(args[i]); 
    }
    
    for(auto && i: options){ 
        RSTR c = i.cmd, v = i.val;
        auto add = [&](int this_flag, RSTR op_str,int ignore_flags) {if(striseq(c+1,op_str))   { IgnoreFlags(c,opFlags,ignore_flags ); opFlags |= this_flag; return true;}else return false;}; 
        if(add(op_header,    "hr"  ,op_lib32 | op_lib64  | op_print   | op_printn)){continue;}
        if(add(op_hex,       "hx"  ,op_bin   | op_lib32  | op_lib64)){continue;}
        if(add(op_bin,       "bn"  ,op_hex   | op_lib32  | op_lib64)){continue;}
        if(add(op_justify,   "j"   ,op_lib32 | op_lib64)){continue;}
        if(add(op_compressed,"c"   ,op_none)){continue;}
        if(add(op_lib32,     "l32" ,op_print | op_printn | op_justify | op_hex  | op_bin | op_lib64 | op_header)){continue;}
        if(add(op_lib64,     "l64" ,op_print | op_printn | op_justify | op_hex  | op_bin | op_lib32 | op_header)){continue;}

        if(add(op_printn,    "pn"  ,op_libs  | op_print  | op_printc  | op_out  | op_header)){continue;}
        if(add(op_print,     "p"   ,op_libs  | op_printn | op_printc  | op_out  | op_header)){continue;}
        if(add(op_printc,    "pc"  ,op_libs  | op_printn | op_print   | op_out  | op_header | op_mods)){continue;}
        //edit options
        if(add(op_out,       "out" ,op_prints)){ outputpath = std::string(v).append(1, PATH_SEPARATOR); continue;}
        if(add(op_jl,        "jl"  ,op_none)){if(!sscanf(v, "%i", &justifyval)) ASSERT(true,"Number expected for editing justify level" );  continue;}

        if(add(op_fcompress,  "cmf" ,op_none)){ 
            tocom = FileRead(v,BIN_READ);   
            ComProc cp(tocom.m_size,tocom.m_data,strffn(v));
            std::string temop = outputpath;
            FileWrite fw(temop.append(strfn(v)).append(".cm").c_str(),BIN_WRITE);
            fw.write((RSTR)cp.m_data,cp.m_size);
            fw.close(); cp.freedat(); tocom.freedat();
            printf("File Compressed at: %s", temop.c_str());
            continue;
        }
        if(add(op_funcompress,"ucf" ,op_none)){ 
            toucm = FileRead(v,BIN_READ); 
            DcomProc dcp(toucm.m_data);
            std::string temop = outputpath;
            FileWrite fw(temop.append(dcp.name).c_str(),BIN_WRITE);
            fw.write((RSTR)dcp.m_data,dcp.m_size);
            fw.close(); dcp.freedat(); tocom.freedat();
            printf("File Decompressed at: %s", temop.c_str());
            continue;
        }
        ASSERT(true,"Option '" << i.cmd <<"' doesn't exist" );  
    }
    
    if( files.empty() && opFlags & (op_funcompress | op_fcompress | op_out) )
        exit(EXIT_SUCCESS); 
    
    if(files.empty()) ASSERT(true,"Coudn't find a file. Make sure the order is valid" ); //! should be bypassed in a single file com proc

    //default flag
    if( !(opFlags & op_writeops) ) opFlags |= op_header;

    std::string headerf=outputpath+"Resources.h", libf = outputpath+(opFlags & op_lib64 ? "libdata64.a" : "libdata32.a");
    //open header file stream
    if(!(opFlags & op_prints)){
        hrwrite.open(headerf.c_str(),TXT_WRITE);
        hrwrite() << "// auto-generated using BinPack. https://github.com/TheOathMan/BinPack\n"
                  << "//IMPORTANT: You must #define BP_INSER_RESOURCES only once before including this header file " 
                  << "in order to insert the data into the compilation process\n";
    }

    //open lib file stream
    if(opFlags & op_libs){
        libWrite.open(libf.c_str(),BIN_WRITE);
        spec = LibGen::GetLibSec(opFlags & op_lib64 ? arc_x64:arc_x86);
        libWrite.write((char*)spec.p1,spec.s1);
    }

    //start header file 
    if(opFlags & op_header) {
        for (auto&& i : files){
            FileRead f(i,BIN_READ,true);
            std::string sfn = strfn(i);
            path2_c_fmt(sfn);
            hrwrite() << "extern const unsigned char BP_"<< sfn << "[" << std::to_string(f.m_size) << "];" << std::endl;
        }
        hrwrite() << "#ifdef BP_INSER_RESOURCES\n";
    }

    for (auto&& i: files){

        ComProc cp;
        frd =  FileRead(i,BIN_READ);
        ASSERT(frd.m_size >= MAX_SIZE, " Size of file: " << i  << " is too big" );
        ASSERT(!frd.m_data,"No data in: " << i);
      
        if(opFlags & op_compressed){
            cp = ComProc(frd.m_size,frd.m_data);                
            printf("%u",cp.m_size);
            PackData(opFlags & op_prints? std::cout : hrwrite(), cp.m_data, cp.m_size, strfn(i), opFlags,justifyval);
            printf("\nsize of '%s' file is: %i bytes. compressed size: %i bytes\n", i, frd.m_size,cp.m_size);        
        } else {
            PackData(opFlags & op_prints ? std::cout : hrwrite(), frd.m_data, frd.m_size, strfn(i), opFlags,justifyval);
            printf("\nsize of '%s' file is: %i bytes \n", i, frd.m_size);
        }
        totalfsize += (opFlags & op_compressed) ? cp.m_size : frd.m_size;
        frd.freedat();
    }
    
    // close header
    if(opFlags & op_header) 
        hrwrite() << "\n#endif // BP_INSER_RESOURCES"; 
    
    //end libgen process 
    if(opFlags & op_libs){
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
        libWrite.close(OnLibWriteClose);
        spec.freep();
    }
    hrwrite.close();
    exit(EXIT_SUCCESS);
}