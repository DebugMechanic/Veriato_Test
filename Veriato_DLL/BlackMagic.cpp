#include "BlackMagic.h"
#include "Log.h"


BLACKMAGIC_MODEL::BLACKMAGIC_MODEL()
{
	// Constructor	
	this->pPatchInfo = (PPATCH_INFO)malloc(sizeof(PATCH_INFO));
	memset(this->pPatchInfo, 0xEE, sizeof(PATCH_INFO));
}


BLACKMAGIC_MODEL::~BLACKMAGIC_MODEL()
{
	// Destructor
	//free(this->pPatchInfo); // Holds Our Patch Work. If you free it. The hook won't work.
}


void BLACKMAGIC_MODEL::Runtime_Dynamic_Linking(char* pFunctionName, char* pDllName)
{
	BYTE* pFunctionPtr = (BYTE*)GetProcAddress(GetModuleHandle(pDllName), pFunctionName);
	this->pPatchInfo->SystemCall = pFunctionPtr;

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->SystemCall: %016llX\n", this->pPatchInfo->SystemCall);
#endif
}


void BLACKMAGIC_MODEL::Define_Original_Signature(int size, char* pSignature, char* mask)
{
	this->pPatchInfo->SignatureSize = size;
	memcpy(this->pPatchInfo->Signature, pSignature, size);
	memcpy(this->pPatchInfo->SignatureMask, mask, size);

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->SignatureSize: %d\n", this->pPatchInfo->SignatureSize);
	Log("pPatchInfo->Signature: %p\n", this->pPatchInfo->Signature);
	Log("pPatchInfo->SignatureMask: %p\n", this->pPatchInfo->SignatureMask);
#endif
}


void BLACKMAGIC_MODEL::Setup_Detour_Patch(int PatchOffset, int SizeOfPatch, char* pPatch )
{
	this->pPatchInfo->PatchOffset = PatchOffset;
	this->pPatchInfo->PatchSize   = SizeOfPatch;
	memcpy(this->pPatchInfo->Patch, pPatch, SizeOfPatch);

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->PatchOffset: %d\n", this->pPatchInfo->PatchOffset);
	Log("pPatchInfo->PatchSize: %d\n", this->pPatchInfo->PatchSize);
	Log("pPatchInfo->Patch With CafeBabe: %p\n", this->pPatchInfo->Patch);
#endif
}


void BLACKMAGIC_MODEL::Assign_Detour(BYTE* function)
{	
	this->pPatchInfo->Detour = function;

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->Detour %p\n", this->pPatchInfo->Detour);
#endif
}


BOOL BLACKMAGIC_MODEL::Set_Memory_Permissions_On_SystemCall(int offset, int permissions)
{	
	DWORD dwOld_Protect;
	if (VirtualProtect(this->pPatchInfo->SystemCall+offset, this->pPatchInfo->SignatureSize, permissions, &dwOld_Protect))
	{
		return 1;
	}
	return 0;
}


bool BLACKMAGIC_MODEL::Verify_Signature(int offset)
{
	BYTE temp[128] = {0};
	int count = 0;
	DWORD i;

	memcpy(&temp, this->pPatchInfo->SystemCall + offset, this->pPatchInfo->SignatureSize);

	for (i = 0; i < this->pPatchInfo->SignatureSize; i++)
	{
		if (this->pPatchInfo->SignatureMask[i] == 'x')
		{
			if (temp[i] != this->pPatchInfo->Signature[i])
			{
				return false;
			}
			count++;
		}
	}

	return true;	
}


void BLACKMAGIC_MODEL::Save_Original_Bytes(int offset)
{	
	memcpy(this->pPatchInfo->Original, this->pPatchInfo->SystemCall + offset, this->pPatchInfo->PatchSize);

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->Original: %p\n", this->pPatchInfo->Original);
#endif
}


void BLACKMAGIC_MODEL::Detour_CafeBabe_Fixup(int offset)
{	
	DWORD64  dwAddress;
	DWORD64* pdwNewAddress;

	dwAddress = (DWORD64)this->pPatchInfo->Detour;              // Copy Address of newRoutine into dwAddress.
	pdwNewAddress = (DWORD64*)&this->pPatchInfo->Patch[offset]; // Copy Address to the first byte of cafebabe.
	*pdwNewAddress = dwAddress;                                 // Assign new Address to the dereferenced address of cafebabe.

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->Patch CafeBabe After Fixup: %p\n", this->pPatchInfo->Patch);
#endif
}


void BLACKMAGIC_MODEL::Setup_Landing_Point(int offset, DWORD64 address)
{
	if (address)
	{
		this->pPatchInfo->LandingPadOffset = 0x0; // This overwrites the original bytes, so we just setup a return.
		this->pPatchInfo->LandingPoint = (BYTE*)address;
	} else {
		this->pPatchInfo->LandingPadOffset = offset;
		this->pPatchInfo->LandingPoint = this->pPatchInfo->SystemCall + this->pPatchInfo->LandingPadOffset;
	}

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->LandingPoint: %p\n", this->pPatchInfo->LandingPoint);
#endif
}


BOOL BLACKMAGIC_MODEL::Set_Memory_Permissions_On_StolenBytes(int permissions)
{
	DWORD dwOld_Protect;
	if (VirtualProtect(this->pPatchInfo->StolenBytes, SZ_PATCH_MAX, permissions, &dwOld_Protect))
	{
		return 1;
	}
	return 0;
}


void BLACKMAGIC_MODEL::Save_Stolen_Bytes()
{
	memcpy(this->pPatchInfo->StolenBytes, this->pPatchInfo->Original, this->pPatchInfo->PatchSize);

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->StolenBytes: %p\n", this->pPatchInfo->StolenBytes);
#endif
}


void BLACKMAGIC_MODEL::Setup_StolenByte_Detour_Patch(char* pPatch, int PatchSize, int offset)
{
	this->pPatchInfo->LandingPadOffset = offset;
	memcpy(this->pPatchInfo->StolenBytes + offset, pPatch, PatchSize);

#if DEBUG_BLACKMAGIC
	Log("pPatchInfo->StolenBytes Patch w/CafeBabe: %p\n", this->pPatchInfo->StolenBytes + offset);
#endif
}


void BLACKMAGIC_MODEL::StolenByte_CafeBabe_Fixup(int offset)
{	
	DWORD64  dwAddress;
	DWORD64* pdwNewAddress;

	dwAddress = (DWORD64)this->pPatchInfo->LandingPoint;              // Copy Address of newRoutine into dwAddress.
	pdwNewAddress = (DWORD64*)&this->pPatchInfo->StolenBytes[offset]; // Copy Address to the first byte of cafebabe.
	*pdwNewAddress = dwAddress;                                       // Assign new Address to the dereferenced address of cafebabe.

#if DEBUG_BLACKMAGIC	
	Log("pPatchInfo->StolenBytes After CafeBabe Fixup: %p\n", this->pPatchInfo->StolenBytes);
#endif
}


DWORD64 BLACKMAGIC_MODEL::Get_Return_Address_To_Original()
{
	return (DWORD64)this->pPatchInfo->StolenBytes;
}


void BLACKMAGIC_MODEL::Activate_Detour()
{	
	DWORD i;
	for (i = 0; i < pPatchInfo->PatchSize; i++){
		pPatchInfo->SystemCall[i + pPatchInfo->PatchOffset] = pPatchInfo->Patch[i];
	}	
}

