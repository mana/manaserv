/*
   AngelCode Scripting Library
   Copyright (c) 2003-2011 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_scriptengine.h
//
// The implementation of the script engine interface
//



#ifndef AS_SCRIPTENGINE_H
#define AS_SCRIPTENGINE_H

#include "as_config.h"
#include "as_atomic.h"
#include "as_scriptfunction.h"
#include "as_array.h"
#include "as_datatype.h"
#include "as_objecttype.h"
#include "as_module.h"
#include "as_restore.h"
#include "as_callfunc.h"
#include "as_configgroup.h"
#include "as_memory.h"
#include "as_gc.h"

BEGIN_AS_NAMESPACE

class asCBuilder;
class asCContext;

// TODO: import: Remove this when import is removed
struct sBindInfo;

// TODO: Deprecate CreateScriptObject. Objects should be created by calling the factory function instead.
// TODO: Deprecate GetSizeOfPrimitiveType. This function is not necessary now that all primitive types have fixed typeIds


// TODO: DiscardModule should take an optional pointer to asIScriptModule instead of module name. If null, nothing is done.

// TODO: Should have a CreateModule/GetModule instead of just GetModule with parameters.

// TODO: Should allow enumerating modules, in case they have not been named.


class asCScriptEngine : public asIScriptEngine
{
//=============================================================
// From asIScriptEngine
//=============================================================
public:
	// Memory management
	virtual int AddRef() const;
	virtual int Release() const;

	// Engine properties
	virtual int     SetEngineProperty(asEEngineProp property, asPWORD value);
	virtual asPWORD GetEngineProperty(asEEngineProp property) const;

	// Compiler messages
	virtual int SetMessageCallback(const asSFuncPtr &callback, void *obj, asDWORD callConv);
	virtual int ClearMessageCallback();
	virtual int WriteMessage(const char *section, int row, int col, asEMsgType type, const char *message);

    // JIT Compiler
    virtual int SetJITCompiler(asIJITCompiler *compiler);
    virtual asIJITCompiler *GetJITCompiler() const;

	// Global functions
	virtual int RegisterGlobalFunction(const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);
	// TODO: interface: Should return asUINT
	virtual int GetGlobalFunctionCount() const;
	virtual int GetGlobalFunctionIdByIndex(asUINT index) const;
	// TODO: interface: Needs GetGlobalFunctionIdByDecl(), just like for module

	// Global properties
	virtual int RegisterGlobalProperty(const char *declaration, void *pointer);
	// TODO: interface: Should return asUINT
	virtual int GetGlobalPropertyCount() const;
	virtual int GetGlobalPropertyByIndex(asUINT index, const char **name, int *typeId = 0, bool *isConst = 0, const char **configGroup = 0, void **pointer = 0) const;
	
	// Type registration
	virtual int            RegisterObjectType(const char *obj, int byteSize, asDWORD flags);
	virtual int            RegisterObjectProperty(const char *obj, const char *declaration, int byteOffset);
	virtual int            RegisterObjectMethod(const char *obj, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);
	virtual int            RegisterObjectBehaviour(const char *obj, asEBehaviours behaviour, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);
	virtual int            RegisterInterface(const char *name);
	virtual int            RegisterInterfaceMethod(const char *intf, const char *declaration);
	// TODO: interface: Should return asUINT
	virtual int            GetObjectTypeCount() const;
	virtual asIObjectType *GetObjectTypeByIndex(asUINT index) const;
	
	// String factory
	virtual int RegisterStringFactory(const char *datatype, const asSFuncPtr &factoryFunc, asDWORD callConv);
	virtual int GetStringFactoryReturnTypeId() const;

	// Default array type
	virtual int RegisterDefaultArrayType(const char *type);
	virtual int GetDefaultArrayTypeId() const;

	// Enums
	virtual int         RegisterEnum(const char *type);
	virtual int         RegisterEnumValue(const char *type, const char *name, int value);
	// TODO: interface: Should return asUINT
	virtual int         GetEnumCount() const;
	virtual const char *GetEnumByIndex(asUINT index, int *enumTypeId, const char **configGroup = 0) const;
	virtual int         GetEnumValueCount(int enumTypeId) const;
	virtual const char *GetEnumValueByIndex(int enumTypeId, asUINT index, int *outValue) const;

	// Funcdefs
	virtual int                RegisterFuncdef(const char *decl);
	// TODO: interface: Should return asUINT
	virtual int                GetFuncdefCount() const;
	virtual asIScriptFunction *GetFuncdefByIndex(asUINT index, const char **configGroup = 0) const;

	// Typedefs
	virtual int         RegisterTypedef(const char *type, const char *decl);
	// TODO: interface: Should return asUINT
	virtual int         GetTypedefCount() const;
	virtual const char *GetTypedefByIndex(asUINT index, int *typeId, const char **configGroup = 0) const;

	// Configuration groups
	virtual int BeginConfigGroup(const char *groupName);
	virtual int EndConfigGroup();
	virtual int RemoveConfigGroup(const char *groupName);
	virtual int SetConfigGroupModuleAccess(const char *groupName, const char *module, bool hasAccess);

	// Script modules
	virtual asIScriptModule *GetModule(const char *module, asEGMFlags flag);
	virtual int              DiscardModule(const char *module);

	// Script functions
	virtual asIScriptFunction *GetFunctionDescriptorById(int funcId) const;

	// Type identification
	virtual asIObjectType *GetObjectTypeById(int typeId) const;
	virtual int            GetTypeIdByDecl(const char *decl) const;
	virtual const char    *GetTypeDeclaration(int typeId) const;
	virtual int            GetSizeOfPrimitiveType(int typeId) const;

	// Script execution
	virtual asIScriptContext *CreateContext();
	virtual void             *CreateScriptObject(int typeId);
	virtual void             *CreateScriptObjectCopy(void *obj, int typeId);
	virtual void              CopyScriptObject(void *dstObj, void *srcObj, int typeId);
	virtual void              ReleaseScriptObject(void *obj, int typeId);
	virtual void              AddRefScriptObject(void *obj, int typeId);
	virtual bool              IsHandleCompatibleWithObject(void *obj, int objTypeId, int handleTypeId) const;

	// String interpretation
	virtual asETokenClass ParseToken(const char *string, size_t stringLength = 0, int *tokenLength = 0) const;

	// Garbage collection
	virtual int  GarbageCollect(asDWORD flags = asGC_FULL_CYCLE);
	virtual void GetGCStatistics(asUINT *currentSize, asUINT *totalDestroyed, asUINT *totalDetected, asUINT *newObjects, asUINT *totalNewDestroyed) const;
	virtual void NotifyGarbageCollectorOfNewObject(void *obj, int typeId);
	virtual void GCEnumCallback(void *reference);

	// User data
	virtual void *SetUserData(void *data);
	virtual void *GetUserData() const;
	virtual void  SetEngineUserDataCleanupCallback(asCLEANENGINEFUNC_t callback);
	virtual void  SetContextUserDataCleanupCallback(asCLEANCONTEXTFUNC_t callback);
	virtual void  SetFunctionUserDataCleanupCallback(asCLEANFUNCTIONFUNC_t callback);

//===========================================================
// internal methods
//===========================================================
public:
	asCScriptEngine();
	virtual ~asCScriptEngine();

//protected:
	friend class asCBuilder;
	friend class asCCompiler;
	friend class asCContext;
	friend class asCDataType;
	friend class asCModule;
	friend class asCRestore;
	friend class asCByteCode;
	friend int PrepareSystemFunction(asCScriptFunction *func, asSSystemFunctionInterface *internal, asCScriptEngine *engine);

	int RegisterMethodToObjectType(asCObjectType *objectType, const char *declaration, const asSFuncPtr &funcPointer, asDWORD callConv);
	int RegisterBehaviourToObjectType(asCObjectType *objectType, asEBehaviours behaviour, const char *decl, const asSFuncPtr &funcPointer, asDWORD callConv);

	int VerifyVarTypeNotInFunction(asCScriptFunction *func);

	void *CallAlloc(asCObjectType *objType) const;
	void  CallFree(void *obj) const;
	void *CallGlobalFunctionRetPtr(int func);
	void *CallGlobalFunctionRetPtr(int func, void *param1);
	void *CallGlobalFunctionRetPtr(asSSystemFunctionInterface *func, asCScriptFunction *desc);
	void *CallGlobalFunctionRetPtr(asSSystemFunctionInterface *i, asCScriptFunction *s, void *param1);
	void  CallObjectMethod(void *obj, int func);
	void  CallObjectMethod(void *obj, void *param, int func);
	void  CallObjectMethod(void *obj, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	void  CallObjectMethod(void *obj, void *param, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	bool  CallObjectMethodRetBool(void *obj, int func);
	int   CallObjectMethodRetInt(void *obj, int func);
	void  CallGlobalFunction(void *param1, void *param2, asSSystemFunctionInterface *func, asCScriptFunction *desc);
	bool  CallGlobalFunctionRetBool(void *param1, void *param2, asSSystemFunctionInterface *func, asCScriptFunction *desc);

	void ConstructScriptObjectCopy(void *mem, void *obj, asCObjectType *type);

	void ClearUnusedTypes();
	void RemoveTemplateInstanceType(asCObjectType *t);
	void RemoveTypeAndRelatedFromList(asCArray<asCObjectType*> &types, asCObjectType *ot);

	asCConfigGroup *FindConfigGroupForFunction(int funcId) const;
	asCConfigGroup *FindConfigGroupForGlobalVar(int gvarId) const;
	asCConfigGroup *FindConfigGroupForObjectType(const asCObjectType *type) const;
	asCConfigGroup *FindConfigGroupForFuncDef(asCScriptFunction *funcDef) const;

	int  RequestBuild();
	void BuildCompleted();

	void PrepareEngine();
	bool isPrepared;

	int CreateContext(asIScriptContext **context, bool isInternal);

	asCObjectType *GetObjectType(const char *type);

	int AddBehaviourFunction(asCScriptFunction &func, asSSystemFunctionInterface &internal);

	asCString GetFunctionDeclaration(int funcID);

	asCScriptFunction *GetScriptFunction(int funcID) const;

	asCModule *GetModule(const char *name, bool create);
	asCModule *GetModuleFromFuncId(int funcId);

	int  GetMethodIdByDecl(const asCObjectType *ot, const char *decl, asCModule *mod);
	int  GetFactoryIdByDecl(const asCObjectType *ot, const char *decl);

	int  GetNextScriptFunctionId();
	void SetScriptFunction(asCScriptFunction *func);
	void FreeScriptFunctionId(int id);

	int ConfigError(int err);

	int                GetTypeIdFromDataType(const asCDataType &dt) const;
	const asCDataType *GetDataTypeFromTypeId(int typeId) const;
	asCObjectType     *GetObjectTypeFromTypeId(int typeId) const;
	void               RemoveFromTypeIdMap(asCObjectType *type);

	bool               IsTemplateType(const char *name) const;
	asCObjectType     *GetTemplateInstanceType(asCObjectType *templateType, asCDataType &subType);
	asCScriptFunction *GenerateTemplateFactoryStub(asCObjectType *templateType, asCObjectType *templateInstanceType, int origFactoryId);
	bool               GenerateNewTemplateFunction(asCObjectType *templateType, asCObjectType *templateInstanceType, asCDataType &subType, asCScriptFunction *templateFunc, asCScriptFunction **newFunc);

	// String constants
	// TODO: Must free unused string constants, thus the ref count for each must be tracked
	int              AddConstantString(const char *str, size_t length);
	const asCString &GetConstantString(int id);

	// Global property management
	asCGlobalProperty *AllocateGlobalProperty();
	void FreeUnusedGlobalProperties();

	int GetScriptSectionNameIndex(const char *name);

//===========================================================
// internal properties
//===========================================================
	asCMemoryMgr memoryMgr;

	int initialContextStackSize;

	asCObjectType   *defaultArrayObjectType;
	asCObjectType    scriptTypeBehaviours;
	asCObjectType    functionBehaviours;
	asCObjectType    objectTypeBehaviours;
	asCObjectType    globalPropertyBehaviours;

	// Registered interface
	asCArray<asCObjectType *>      registeredObjTypes;
	asCArray<asCObjectType *>      registeredTypeDefs;
	asCArray<asCObjectType *>      registeredEnums;
	asCArray<asCGlobalProperty *>  registeredGlobalProps;
	asCArray<asCScriptFunction *>  registeredGlobalFuncs;
	asCArray<asCScriptFunction *>  registeredFuncDefs;
	asCScriptFunction             *stringFactory;
	bool configFailed;

	// Stores all known object types, both application registered, and script declared
	asCArray<asCObjectType *>      objectTypes;
	asCArray<asCObjectType *>      templateSubTypes;

	// Store information about template types
	asCArray<asCObjectType *>      templateTypes;

	// Stores all global properties, both those registered by application, and those declared by scripts.
	// The id of a global property is the index in this array.
	asCArray<asCGlobalProperty *> globalProperties;
	asCArray<int>                 freeGlobalPropertyIds;

	// Stores all functions, i.e. registered functions, script functions, class methods, behaviours, etc.
	asCArray<asCScriptFunction *> scriptFunctions;
	asCArray<int>                 freeScriptFunctionIds;
	asCArray<asCScriptFunction *> signatureIds;

	// An array with all module imported functions
	asCArray<sBindInfo *>  importedFunctions;
	asCArray<int>          freeImportedFunctionIdxs;

	// These resources must be protected for multiple accesses
	mutable asCAtomic      refCount;
	asCArray<asCModule *>  scriptModules;
	asCModule             *lastModule;
	bool                   isBuilding;

	// Stores script declared object types
	asCArray<asCObjectType *> classTypes;
	// This array stores the template instances types, that have been generated from template types
	asCArray<asCObjectType *> templateInstanceTypes;
	// Stores the funcdefs
	asCArray<asCScriptFunction *> funcDefs;

	// Stores the names of the script sections for debugging purposes
	asCArray<asCString *> scriptSectionNames;

	// Type identifiers
	mutable int                       typeIdSeqNbr;
	mutable asCMap<int, asCDataType*> mapTypeIdToDataType;

	// Garbage collector
	asCGarbageCollector gc;

	// Dynamic groups
	asCConfigGroup             defaultGroup;
	asCArray<asCConfigGroup*>  configGroups;
	asCConfigGroup            *currentGroup;

	// Message callback
	bool                        msgCallback;
	asSSystemFunctionInterface  msgCallbackFunc;
	void                       *msgCallbackObj;

    asIJITCompiler              *jitCompiler;

	// String constants
	asCArray<asCString*>        stringConstants;

	// User data
	void                  *userData;
	asCLEANENGINEFUNC_t    cleanEngineFunc;
	asCLEANCONTEXTFUNC_t   cleanContextFunc;
	asCLEANFUNCTIONFUNC_t  cleanFunctionFunc;

	// Critical sections for threads
	DECLARECRITICALSECTION(engineCritical);

	// Engine properties
	struct
	{
		bool allowUnsafeReferences;
		bool optimizeByteCode;
		bool copyScriptSections;
		int  maximumContextStackSize;
		bool useCharacterLiterals;
		bool allowMultilineStrings;
		bool allowImplicitHandleTypes;
		bool buildWithoutLineCues;
		bool initGlobalVarsAfterBuild;
		bool requireEnumScope;
		int  scanner;
		bool includeJitInstructions;
		int  stringEncoding;
		int  propertyAccessorMode;
		bool expandDefaultArrayToTemplate;
		bool autoGarbageCollect;
	} ep;
};

END_AS_NAMESPACE

#endif
