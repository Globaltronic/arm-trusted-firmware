/*
 * Copyright (c) 2014, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <debug.h>
#include <psci.h>
#include <runtime_svc.h>
#include <std_svc.h>
#include <stdint.h>
#include <uuid.h>
#include <assert.h>
#include <string.h>
#include <bl_common.h>
#include <context_mgmt.h>
#include <mmio.h>

#define ARM_NUM_CALLS 4

#define ARM_SVC_CALL_COUNT	0x8000ff00
#define ARM_SVC_UID		0x8000ff01
//0x8000ff02 reserved
#define ARM_SVC_VERSION		0x8000ff03
#define ARM_SVC_RUNNSOS		0x8000ff04

/* ARM Standard Service Calls version numbers */
#define ARM_SVC_VERSION_MAJOR		0x0
#define ARM_SVC_VERSION_MINOR		0x1

/* Arm arch Service UUID */

DEFINE_SVC_UUID(arm_svc_uid,
		0x83B53A5B, 0xC594, 0x40B9, 0x53, 0x91,
		0xF2, 0xFC, 0x54, 0x29, 0x86, 0x48);
/*******************************************************************************
 * This function programs EL3 registers and performs other setup to enable entry
 * into the next image after BL31 at the next ERET.
 ******************************************************************************/
void prepare_nonsec_os_entry(uint64_t kernel_addr, uint64_t dtb_addr)
{
	entry_point_info_t next_image_info;
	uint32_t image_type;

	/* Determine which image to execute next */
	image_type = NON_SECURE;

	/* Program EL3 registers to enable entry into the next EL */
	memset(&next_image_info, 0, sizeof(next_image_info));
	SET_SECURITY_STATE(next_image_info.h.attr, NON_SECURE);
	next_image_info.spsr = SPSR_64(MODE_EL2, MODE_SP_ELX,
				       DISABLE_ALL_EXCEPTIONS);
	next_image_info.pc = kernel_addr;
	next_image_info.args.arg0 = dtb_addr;
	
	

	INFO("BL3-1: Next image address = 0x%llx\n",(unsigned long long) next_image_info.pc);
	INFO("BL3-1: Next image spsr = 0x%x\n", next_image_info.spsr);

	cm_init_context(read_mpidr_el1(), &next_image_info);
	cm_prepare_el3_exit(image_type);
}


/*
 * Top-level Standard Service SMC handler. This handler will in turn dispatch
 * calls to PSCI SMC handler
 */
uint64_t arm_svc_smc_handler(uint32_t smc_fid,
			     uint64_t x1,
			     uint64_t x2,
			     uint64_t x3,
			     uint64_t x4,
			     void *cookie,
			     void *handle,
			     uint64_t flags)
{
	//INFO(" Arm Architechture Service Call: 0x%x \n", smc_fid);
	
	switch (smc_fid) {
	case ARM_SVC_CALL_COUNT:
		SMC_RET1(handle, ARM_NUM_CALLS);
	case ARM_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, arm_svc_uid);
	case ARM_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, ARM_SVC_VERSION_MAJOR, ARM_SVC_VERSION_MINOR);
	case ARM_SVC_RUNNSOS:
		prepare_nonsec_os_entry((uint32_t)x1,(uint32_t)x2);
		SMC_RET0(handle);

	default:
		WARN("Unimplemented Standard Service Call: 0x%x \n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(
		arm_svc,

		OEN_ARM_START,
		OEN_ARM_END,
		SMC_TYPE_FAST,
		NULL,
		arm_svc_smc_handler
);
