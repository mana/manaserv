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
// as_objecttype.h
//
// A class for storing object type information
//



#ifndef AS_OBJECTTYPE_H
#define AS_OBJECTTYPE_H

#include "as_atomic.h"
#include "as_string.h"
#include "as_property.h"
#include "as_array.h"
#include "as_scriptfunction.h"

BEGIN_AS_NAMESPACE

// TODO: memory: Need to minimize used memory here, because not all types use all properties of the class

// TODO: The type id should have flags for diferenciating between value types and reference types. It should also have a flag for differenciating interface types.

// Additional flag to the class object type
const asDWORD asOBJ_IMPLICIT_HANDLE  = 0x00400000;
const asDWORD asOBJ_TYPEDEF          = 0x40000000;
const asDWORD asOBJ_ENUM             = 0x10000000;
const asDWORD asOBJ_TEMPLATE_SUBTYPE = 0x20000000;




// asOBJ_GC is used to indicate that the type can potentially 
// form circular references, thus is garbage collected.

// The fact that an object is garbage collected doesn't imply that an object that 
// can references it also must be garbage collected, only if the garbage collected 
// object can reference it as well.

// For registered types however, we set the flag asOBJ_GC if the GC 
// behaviours are registered. For script types that contain any such type we 
// automatically make garbage collected as well, because we cannot know what type
// of references that object can contain, and must assume the worst.

// TODO: interface: asIObjectType should have methods for setting/getting user data
//                  this can for example be used to store cached function ids for quicker
//                  executions of class methods

struct asSTypeBehaviour
{
	asSTypeBehaviour() 
	{
		factory = 0;
		listFactory = 0;
		copyfactory = 0;
		construct = 0; 
		copyconstruct = 0;
		destruct = 0; 
		copy = 0; 
		addref = 0; 
		release = 0; 
		gcGetRefCount = 0; 
		gcSetFlag = 0; 
		gcGetFlag = 0; 
		gcEnumReferences = 0; 
		gcReleaseAllReferences = 0;
		templateCallback = 0;
	}

	int factory;
	int listFactory; // Used for initialization lists only
	int copyfactory;
	int construct;
	int copyconstruct;
	int destruct;
	int copy;
	int addref;
	int release;
	int templateCallback;
	
	// GC behaviours
	int gcGetRefCount;
	int gcSetFlag;
	int gcGetFlag;
	int gcEnumReferences;
	int gcReleaseAllReferences;
	
	asCArray<int> factories;
	asCArray<int> constructors;
	asCArray<int> operators;
};

struct asSEnumValue
{
	asCString name;
	int       value;
};

class asCScriptEngine;

void RegisterObjectTypeGCBehaviours(asCScriptEngine *engine);

class asCObjectType : public asIObjectType
{
public:
//=====================================
// From asIObjectType
//=====================================
	asIScriptEngine *GetEngine() const;
	const char      *GetConfigGroup() const;

	// Memory management
	int AddRef() const;
	int Release() const;

	// Type info
	const char      *GetName() const;
	asIObjectType   *GetBaseType() const;
	asDWORD          GetFlags() const;
	asUINT           GetSize() const;
	int              GetTypeId() const;
	int              GetSubTypeId() const;

	// Interfaces
	asUINT           GetInterfaceCount() const;
	asIObjectType   *GetInterface(asUINT index) const;

	// Factories
	asUINT             GetFactoryCount() const;
	int                GetFactoryIdByIndex(asUINT index) const;
	int                GetFactoryIdByDecl(const char *decl) const;

	// Methods
	asUINT             GetMethodCount() const;
	int                GetMethodIdByIndex(asUINT index, bool getVirtual) const;
	int                GetMethodIdByName(const char *name, bool getVirtual) const;
	int                GetMethodIdByDecl(const char *decl, bool getVirtual) const;
	asIScriptFunction *GetMethodDescriptorByIndex(asUINT index, bool getVirtual) const;

	// Properties
	asUINT      GetPropertyCount() const;
	int         GetProperty(asUINT index, const char **name, int *typeId, bool *isPrivate, int *offset, bool *isReference) const;
	const char *GetPropertyDeclaration(asUINT index) const;

	// Behaviours
	asUINT GetBehaviourCount() const;
	int    GetBehaviourByIndex(asUINT index, asEBehaviours *outBehaviour) const;

//===========================================
// Internal
//===========================================
public:
	asCObjectType(); 
	asCObjectType(asCScriptEngine *engine);
	~asCObjectType();

	int  GetRefCount();
	void SetGCFlag();
	bool GetGCFlag();
	void EnumReferences(asIScriptEngine *);
	void ReleaseAllHandles(asIScriptEngine *);

	void ReleaseAllFunctions();

	// TODO: interface: Implements and DerivesFrom could be made public methods in the asIObjectType interface
	bool Implements(const asCObjectType *objType) const;
	bool DerivesFrom(const asCObjectType *objType) const;
	bool IsInterface() const;

	asCObjectProperty *AddPropertyToClass(const asCString &name, const asCDataType &dt, bool isPrivate);

	asCString   name;
	int         size;
	asCArray<asCObjectProperty*> properties;
	asCArray<int>                methods;
	asCArray<asCObjectType*>     interfaces;
	asCArray<asSEnumValue*>      enumValues;
	asCObjectType *              derivedFrom;
	asCArray<asCScriptFunction*> virtualFunctionTable;

	asDWORD flags;

	asSTypeBehaviour beh;

	// Used for template types
	asCDataType    templateSubType;
	bool           acceptValueSubType;
	bool           acceptRefSubType;

	asCScriptEngine *engine;

protected:
	mutable asCAtomic refCount;
	mutable bool      gcFlag;
};

END_AS_NAMESPACE

#endif
