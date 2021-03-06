//
// libembryo
//
// Copyright Stanley Cen 2013
// Released under the MIT License
//

#include "memmgr.h"

#include <libembryo/logger.h>

#include "distorm.h"
#include "mnemonics.h"

namespace embryo
{
    kern_return_t memmgr::getMemoryProtection(void *addr, vm_prot_t &prot)
    {
        vm_region_basic_info_data_t info;
        vm_size_t vmsize;
        vm_address_t address = (vm_address_t)addr;
        mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT;
        memory_object_name_t object;
        kern_return_t status = vm_region(mach_task_self(), &address, &vmsize,
                                         VM_REGION_BASIC_INFO, (vm_region_info_t)&info, &infoCount,
                                         &object);
        prot = info.protection;
        return status;
    }
    
    bool memmgr::writeBytes(void *dst, std::vector<unsigned char>& bytes)
    {
        vm_prot_t prot;
        kern_return_t status = getMemoryProtection(dst, prot);
        if (status)
        {
            log().error(format("vm_region() failed at %X") % dst);
            return 0;
        }
        
        status = vm_protect(mach_task_self(), (vm_address_t)dst, bytes.size(), 0, VM_PROT_ALL);
        
        if (status)
        {
            log().error(format("vm_protect() failed to set %X + %u bytes to VM_PROT_WRITE") % dst % bytes.size());
            return 0;
        }
        
        unsigned char *byteDst = (unsigned char *)dst;
        
        for (int i = 0; i < bytes.size(); i++)
        {
            *(byteDst + i) = bytes[i];
        }
        
        // restore original protection
        vm_protect(mach_task_self(), (vm_address_t)dst, bytes.size(), 0, prot);
        
        return 1;
    }

    std::vector<unsigned char> memmgr::readBytes(void *src, int len)
    {
        std::vector<unsigned char> ret;
        
        for (int i = 0; i < len; i++)
            ret.push_back(*((unsigned char *)src + i));
        
        return ret;
    }
    
    void memmgr::disasm(void *addr, unsigned int numInstructions, unsigned int codeSize)
    {
        _DecodedInst dec[30] = {0};
        unsigned int decCount = 0;
        
        distorm_decode64((_OffsetType)addr, (unsigned char *)addr, codeSize, Decode32Bits, dec, numInstructions, &decCount);
        
        for (int i = 0; i < decCount; i++)
        {
            log().verb(format("%08llx (%02d) %-20s %s%s%s") % dec[i].offset % dec[i].size % dec[i].instructionHex.p % dec[i].mnemonic.p
                       % (dec[i].operands.length != 0 ? " " : "") % dec[i].operands.p);
        }
    }
}