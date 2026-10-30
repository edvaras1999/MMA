#include "Runtime.h"
#include "Signatures.h"
namespace SPII {
bool VerifySites(){
 unsigned mismatch=0,matched=0,detours=0;
 for(uintptr_t va:globalSites)if(!Readable(reinterpret_cast<void*>(va),4)){Log("UNREADABLE global %08X",unsigned(va));return false;}
 for(const auto& site:sites){
  auto* data=reinterpret_cast<const unsigned char*>(site.va);if(!Readable(data,8,false)){Log("UNREADABLE site %s",site.name);++mismatch;continue;}
  if(memcmp(data,site.bytes,8)==0){++matched;continue;}
  unsigned first=0;while(first<8&&data[first]==site.bytes[first])++first;
  bool jump=first<8&&(data[first]==0xE9||data[first]==0xEB||(first<7&&data[first]==0xFF&&(data[first+1]==0x25||data[first+1]==0x15)));
  if(jump){++detours;Log("Existing code hook at %s +%u",site.name,first);}else{++mismatch;Log("WARNING changed bytes at %s",site.name);}
 }
 for(const auto& slot:slotSites){
  auto** at=reinterpret_cast<void**>(slot.table+slot.slot*4);
  if(!Readable(at,4)||!Executable(*at)){Log("UNUSABLE vtable slot %08X+%u",unsigned(slot.table),slot.slot);return false;}
  if(reinterpret_cast<uintptr_t>(*at)!=slot.target)Log("Chaining existing vtable hook %08X+%u",unsigned(slot.table),slot.slot);
 }
 Log("Signatures exact=%u existing detours=%u changed=%u",matched,detours,mismatch);
 return mismatch<=1&&matched+detours>mismatch;
}
}
