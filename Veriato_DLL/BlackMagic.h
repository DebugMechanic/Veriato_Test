#ifndef BLACKMAGIC_H
#define BLACKMAGIC_H

#include <Windows.h>


#define P_SIG_SIZE 0xF                  // Signature Size
#define P_PATCH_SIZE 0xD                // Patch Size of Detour
#define P_PATCH_OFFSET 0x0              // Patch Offset of Detour
#define P_PERM_OFFSET 0x0               // Permission Offset
#define P_PLACEMENT_OFFSET 0x0          // Placement Offset
#define P_ORG_BYTE_OFFSET 0x0           // Original Byte Offset
#define P_CAFEBABE_FIXUP_OFFSET 0x2     // Cafebabe fixup
#define P_LANDINGPOINT_OFFSET 0xD       // Landing Point Offset 
#define P_RTN_PATCH_SIZE 0xD            // Stolen Byte Returning Patch Size
#define P_RTN_PATCH_OFFSET 0xD          // Stolen Byte Returning Patch Offset
#define P_RTN_CAFEBABE_FIXUP_OFFSET 0xF // Stolen Byte Cafebabe Fixup Offset

#define SZ_SIG_MAX   128 /* Maximum size of a Nt*() signature (in bytes) */
#define SZ_PATCH_MAX 32  /* Maximum size of a detour patch (in bytes) */


typedef struct _PATCH_INFO
{
	// Function to Patch
	BYTE* SystemCall;                /* address of routine being patched */

	// Verify Signature 
	BYTE  Signature[SZ_SIG_MAX];     /* byte-signature for sanity check */
	BYTE  SignatureMask[SZ_SIG_MAX]; /* byte-signature mask */
	DWORD SignatureSize;             /* actual size of signature */

	// Hook Information
	BYTE* Detour;                    /* address of prologue detour */
	BYTE  Patch[SZ_PATCH_MAX];       /* jump instructions to detour */
	BYTE  Original[SZ_PATCH_MAX];    /* bytes that were replaced */
	DWORD PatchSize;                 /* size within bytes */
	DWORD PatchOffset;               /* relative location of patch */

	// Stolen Bytes and Return Code
	BYTE  StolenBytes[SZ_PATCH_MAX]; /* Bytes stolen */
	BYTE* LandingPoint;              /* Returning Area of Original Function */
	DWORD LandingPadOffset;          /* Original SystemCall Entry + Offset Return */

} PATCH_INFO, *PPATCH_INFO;


class BLACKMAGIC_MODEL
{
	public:
		BLACKMAGIC_MODEL();  // Constructor
		~BLACKMAGIC_MODEL(); // Destructor

		void Runtime_Dynamic_Linking(char* pFunctionName, char* pDllName);
		void Define_Original_Signature(int size, char* pSignature, char* mask);
		void Setup_Detour_Patch(int PatchOffset, int SizeOfPatch, char* pPatch);
		void Assign_Detour(BYTE* function);
		BOOL Set_Memory_Permissions_On_SystemCall(int offset, int permissions);
		bool Verify_Signature(int offset);
		void Save_Original_Bytes(int offset);
		void Detour_CafeBabe_Fixup(int offset);
		void Setup_Landing_Point(int offset, DWORD64 address);
		BOOL Set_Memory_Permissions_On_StolenBytes(int permissions);
		void Save_Stolen_Bytes();
		void Setup_StolenByte_Detour_Patch(char* pPatch, int PatchSize, int offset);
		void StolenByte_CafeBabe_Fixup(int offset);
		DWORD64 Get_Return_Address_To_Original();
		void Activate_Detour();
		
	private:				
		PPATCH_INFO pPatchInfo;
};


#endif

