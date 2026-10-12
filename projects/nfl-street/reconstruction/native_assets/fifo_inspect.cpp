// Research-only FIFO inspection with the existing local Dolphin opcode decoder.
// This executable is not linked into the independent native game.
#include "VideoCommon/OpcodeDecoding.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
// DFF v6 uses packed little-endian records; GPU command bytes remain big-endian.
// Read only bounded, defined fields, without initializing Dolphin's memory system.
struct Capture {
  std::vector<u8> bytes;
  explicit Capture(const char* path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in)throw std::runtime_error("capture open failed");
    auto size=in.tellg();if(size<128||size>512*1024*1024)throw std::runtime_error("capture size");
    bytes.resize((size_t)size);in.seekg(0);in.read((char*)bytes.data(),size);
    if(!in||word(0)!=0x0d01f1f0||word(4)!=6||word(8)>6)throw std::runtime_error("unsupported DFF header");
  }
  const u8* span(u64 at,u64 size) const {
    if(at>bytes.size()||size>bytes.size()-at)throw std::runtime_error("capture span");
    return bytes.data()+(size_t)at;
  }
  u32 word(u64 at) const {auto p=span(at,4);return p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;}
  u64 wide(u64 at) const {return word(at)|(u64)word(at+4)<<32;}
  void words(u64 offset,u32 count,u32* dest,size_t limit) const {
    if(count>limit)throw std::runtime_error("register count");span(offset,(u64)count*4);
    for(u32 i=0;i<count;++i)dest[i]=word(offset+(u64)i*4);
  }
};
struct Inspector : OpcodeDecoder::Callback {
  CPState cp{};std::array<u32,256> bp{};std::vector<u32> xf;
  std::ostream& out;std::vector<u8> ram,tmem;unsigned draws=0,commands=0;size_t fifo_position=0;bool first=true,actor_data=false,rigid_data=false;
  explicit Inspector(std::ostream& stream):out(stream),xf(5120),ram(32*1024*1024),tmem(1024*1024){}
  void OnXF(u16 address,u8 count,const u8* data) override {
    if((size_t)address+count>xf.size())throw std::runtime_error("XF span");
    for(unsigned i=0;i<count;++i)xf[address+i]=Common::swap32(data+i*4);
  }
  void OnCP(u8 command,u32 value) override {
    // GameCube physical addresses; avoid LoadCPReg's global System/analytics hooks.
    if(command>=0xa0&&command<=0xaf){cp.array_bases[(CPArray)(command&15)]=value&0x1ffffff;return;}
    if(command>=0xb0&&command<=0xbf){cp.array_strides[(CPArray)(command&15)]=value&255;return;}
    if(command==0x20&&value==0)return;
    if(command==0x30||command==0x40||command==0x50||command==0x60||
       (command>=0x70&&command<=0x77)||(command>=0x80&&command<=0x87)||
       (command>=0x90&&command<=0x97)){cp.LoadCPReg(command,value);return;}
    throw std::runtime_error("unsupported CP command");
  }
  void OnBP(u8 command,u32 value) override {
    u32 mask=bp[254]&0xffffff;
    bp[command]=(bp[command]&~mask)|(value&mask);
    if(command!=254)bp[254]=0xffffff;
    if(actor_data&&command==0x65){
      size_t dest=(bp[0x65]&1023)*512,source=(bp[0x64]<<5)&0x1ffffff,n=((bp[0x65]>>10)&2047)*32;
      if(n>tmem.size()-dest||n>ram.size()-source)throw std::runtime_error("TLUT upload span");
      std::copy_n(ram.begin()+source,n,tmem.begin()+dest);
    }
  }
  void OnIndexedLoad(CPArray array,u32 index,u16 address,u8 count) override {
    u32 source=(cp.array_bases[array]+cp.array_strides[array]*index)&0x1ffffff;
    if(source+(size_t)count*4>ram.size()||(size_t)address+count>xf.size())throw std::runtime_error("indexed XF span");
    for(unsigned i=0;i<count;++i)xf[address+i]=Common::swap32(ram.data()+source+i*4);
  }
  void OnPrimitiveCommand(OpcodeDecoder::Primitive primitive,u8 vat,u32 vertex_size,u16 vertices,const u8*) override {
    if(!first)out<<',';first=false;++draws;
    out<<"{\"fifo_position\":"<<fifo_position<<",\"primitive\":"<<(unsigned)primitive<<",\"vat\":"<<(unsigned)vat<<",\"vertices\":"<<vertices<<",\"vertex_size\":"<<vertex_size<<",\"bp\":[";
    for(unsigned i=0;i<256;++i){if(i)out<<',';out<<bp[i];}
    out<<"],\"array_bases\":[";
    for(unsigned i=0;i<16;++i){if(i)out<<',';out<<cp.array_bases[(CPArray)i];}
    out<<"],\"array_strides\":[";
    for(unsigned i=0;i<16;++i){if(i)out<<',';out<<cp.array_strides[(CPArray)i];}
    out<<"],\"projection\":[";
    for(unsigned i=0x1020;i<=0x1026;++i){if(i!=0x1020)out<<',';out<<xf[i];}
    out<<"],\"matrix_index_a\":"<<cp.matrix_index_a.Hex<<",\"position_matrix\":[";
    unsigned matrix=(cp.matrix_index_a.Hex&63)*4;
    for(unsigned i=0;i<12;++i){if(i)out<<',';out<<xf[matrix+i];}
    out<<"],\"vertex_descriptor\":["<<cp.vtx_desc.low.Hex<<','<<cp.vtx_desc.high.Hex
       <<"],\"vertex_format\":["<<cp.vtx_attr[vat].g0.Hex<<','<<cp.vtx_attr[vat].g1.Hex<<','<<cp.vtx_attr[vat].g2.Hex
       <<"],\"xf_registers\":[";
    for(unsigned i=4096;i<4184;++i){if(i!=4096)out<<',';out<<xf[i];}
    out<<"],\"texture_matrix\":[";
    unsigned texmatrix=((cp.matrix_index_a.Hex>>6)&63)*4;
    for(unsigned i=0;i<12;++i){if(i)out<<',';out<<xf[texmatrix+i];}
    out<<"]";
    if(actor_data&&(rigid_data||(cp.vtx_desc.low.Hex&1))){
      out<<",\"position_palette\":[";
      for(unsigned i=0;i<264;++i){if(i)out<<',';out<<xf[i];}
      out<<"],\"normal_palette\":[";
      for(unsigned i=0x400;i<0x500;++i){if(i!=0x400)out<<',';out<<xf[i];}
      out<<"],\"lights\":[";
      for(unsigned i=0x600;i<0x680;++i){if(i!=0x600)out<<',';out<<xf[i];}
      out<<"],\"palettes\":[";
      for(unsigned unit=0;unit<2;++unit){if(unit)out<<',';out<<'[';
        unsigned fmt=(bp[0x88+unit]>>20)&15,n=fmt==8?32:fmt==9?512:0;
        size_t at=(bp[0x98+unit]&1023)*512;
        for(unsigned i=0;i<n;++i){if(i)out<<',';out<<(unsigned)tmem[at+i];}out<<']';
      }
      out<<"],\"secondary_texture_matrix\":[";
      unsigned second=((cp.matrix_index_a.Hex>>12)&63)*4;
      for(unsigned i=0;i<12;++i){if(i)out<<',';out<<xf[second+i];}out<<']';
    }
    out<<"}";
  }
  void OnDisplayList(u32,u32) override {throw std::runtime_error("unexpected non-inlined display list");}
  void OnNop(u32) override {}
  void OnUnknown(u8 opcode,const u8*) override {
    // These single-byte commands have no vertex/BP/XF state effects.
    if(opcode==0x44||opcode==0x48)return;
    throw std::runtime_error("unknown FIFO opcode "+std::to_string(opcode));
  }
  void OnCommand(const u8*,u32) override {++commands;}
  CPState& GetCPState() override {return cp;}
};
int main(int argc,char** argv){
  if(argc!=3&&argc!=4)return 2;
  if(argc==4&&std::string(argv[3])!="--actor-state"&&std::string(argv[3])!="--rigid-state")return 2;
  try {
    Capture file(argv[1]);
    std::ofstream out(argv[2]);if(!out)return 1;Inspector x(out);x.actor_data=argc==4;x.rigid_data=argc==4&&std::string(argv[3])=="--rigid-state";
    std::array<u32,256> cp{};
    file.words(file.wide(12),file.word(20),x.bp.data(),256);
    file.words(file.wide(24),file.word(32),cp.data(),256);
    file.words(file.wide(36),file.word(44),x.xf.data(),4096);
    file.words(file.wide(48),file.word(56),x.xf.data()+4096,x.xf.size()-4096);
    if(x.actor_data){auto n=file.word(84);if(n>x.tmem.size())throw std::runtime_error("TMEM size");
      std::copy_n(file.span(file.wide(76),n),n,x.tmem.begin());}
    for(unsigned cmd:{0x30,0x40,0x50,0x60})x.OnCP((u8)cmd,cp[cmd]);
    for(unsigned group:{0x70,0x80,0x90})for(unsigned i=0;i<8;++i)x.OnCP((u8)(group+i),cp[group+i]);
    for(unsigned group:{0xa0,0xb0})for(unsigned i=0;i<16;++i)x.OnCP((u8)(group+i),cp[group+i]);
    u32 frame_count=file.word(68);u64 frames=file.wide(60);
    if(frame_count>10000)throw std::runtime_error("frame count");file.span(frames,(u64)frame_count*64);
    out<<"{\"frames\":[";
    for(unsigned f=0;f<frame_count;++f){if(f)out<<',';
      u64 record=frames+(u64)f*64,fifo_offset=file.wide(record),updates=file.wide(record+20);
      u32 fifo_size=file.word(record+8),update_count=file.word(record+28);
      auto fifo=file.span(fifo_offset,fifo_size);file.span(updates,(u64)update_count*24);
      x.first=true;x.draws=x.commands=0;size_t at=0,update=0;
      out<<"{\"fifo_bytes\":"<<fifo_size<<",\"memory_updates\":"<<update_count<<",\"draws\":[";
      while(at<fifo_size){
        while(update<update_count&&file.word(updates+(u64)update*24)<=at){u64 m=updates+(u64)update++*24;
          u32 address=file.word(m+4),size=file.word(m+16);
          if(address&0x10000000)throw std::runtime_error("EXRAM unsupported in this GameCube inspector");
          size_t dest=address&0x1ffffff;if(size>x.ram.size()-dest)throw std::runtime_error("memory update span");
          auto data=file.span(file.wide(m+8),size);std::copy_n(data,size,x.ram.begin()+dest);
        }
        x.fifo_position=at;
        u32 used=OpcodeDecoder::RunCommand(fifo+at,(u32)(fifo_size-at),x);
        if(!used)throw std::runtime_error("truncated FIFO command");at+=used;
      }
      out<<"],\"draw_count\":"<<x.draws<<",\"commands\":"<<x.commands<<"}";
    }
    out<<"]}\n";out.flush();return out?0:1;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
