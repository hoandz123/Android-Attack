
//
// Created by TEAMHMG on 02/09/2025.
//


DO_API(void*, il2cpp_alloc, (size_t size));
DO_API(void, il2cpp_free, (void* ptr));


// Domain
DO_API(Il2CppDomain*, il2cpp_domain_get, ());
DO_API(Il2CppThread*, il2cpp_thread_attach, (Il2CppDomain * domain));
DO_API(Il2CppAssembly*, il2cpp_domain_assembly_open, (Il2CppDomain * domain, const char* name));
DO_API(Il2CppAssembly**, il2cpp_domain_get_assemblies, (Il2CppDomain* domain, size_t* size));


DO_API(Il2CppImage*, il2cpp_assembly_get_image, (Il2CppAssembly * assembly));

// Thread
DO_API(bool, il2cpp_is_vm_thread, (Il2CppThread * thread));


// Image
DO_API(const char*, il2cpp_image_get_name, (Il2CppImage* image));
DO_API(size_t, il2cpp_image_get_class_count, (Il2CppImage* image));
DO_API(Il2CppClass*, il2cpp_class_from_name, (Il2CppImage*, const char*, const char*));
DO_API(Il2CppClass*, il2cpp_image_get_class, (Il2CppImage*, size_t));
DO_API(Il2CppAssembly*, il2cpp_image_get_assembly, (Il2CppImage*));

//Method
DO_API(const char*, il2cpp_method_get_name, (Il2CppMethod*));
DO_API(bool, il2cpp_method_is_generic, (Il2CppMethod*));
DO_API(bool, il2cpp_method_is_inflated, (Il2CppMethod*));
DO_API(bool, il2cpp_method_is_instance, (Il2CppMethod*));
DO_API(uint32_t, il2cpp_method_get_param_count, (Il2CppMethod*));
DO_API(Il2CppType*, il2cpp_method_get_param, (Il2CppMethod*, uint32_t));
DO_API(Il2CppClass*, il2cpp_method_get_class, (Il2CppMethod*));
DO_API(const char*, il2cpp_method_get_param_name, (Il2CppMethod*, uint32_t));
DO_API(Il2CppType*, il2cpp_method_get_return_type, (Il2CppMethod*));
DO_API(uint32_t, il2cpp_method_get_flags, (Il2CppMethod * method, uint32_t * iflags));
DO_API(uint32_t, il2cpp_method_get_token, (Il2CppMethod * method));
DO_API(Il2CppObject*, il2cpp_runtime_invoke, (Il2CppMethod*, void*, void**, void**));


// Field
DO_API(void*, il2cpp_field_static_get_value, (Il2CppField*, void*));
DO_API(void, il2cpp_field_static_set_value, (Il2CppField*, void*));
DO_API(size_t, il2cpp_field_get_offset, (Il2CppField*));
DO_API(const char*, il2cpp_field_get_name, (Il2CppField * field));
DO_API(Il2CppType*, il2cpp_field_get_type, (Il2CppField * field));
DO_API(int, il2cpp_field_get_flags, (Il2CppField * field));


// String
DO_API(Il2CppString*, il2cpp_string_new, (const char*));
DO_API(int32_t, il2cpp_string_length, (Il2CppString* str));
DO_API(uint16_t*, il2cpp_string_chars, (Il2CppString* str));

// Object
DO_API(void, il2cpp_field_get_value, (Il2CppObject* obj, Il2CppField* field, void* value));
DO_API(Il2CppObject*, il2cpp_field_get_value_object, (Il2CppField* field, Il2CppObject* obj));
DO_API(void, il2cpp_field_set_value, (Il2CppObject* obj, Il2CppField* field, void* value));
DO_API(void, il2cpp_field_set_value_object, (Il2CppObject* instance, Il2CppField* field, Il2CppObject* value));
DO_API(Il2CppClass*, il2cpp_object_get_class, (Il2CppObject* obj));
DO_API(uint32_t, il2cpp_object_get_size, (Il2CppObject* obj));
DO_API(Il2CppMethod*, il2cpp_object_get_virtual_method, (Il2CppObject* obj, Il2CppMethod* method));
DO_API(void*, il2cpp_object_unbox, (Il2CppObject* obj));
DO_API(void, il2cpp_runtime_object_init, (Il2CppObject * obj));

DO_API(uint32_t, il2cpp_array_length, (Il2CppObject *array));

// type
DO_API(Il2CppClass*, il2cpp_class_from_type, (Il2CppType * type));
DO_API(Il2CppObject*, il2cpp_type_get_object, (Il2CppType* type));
DO_API(int, il2cpp_type_get_type, (Il2CppType* type));
DO_API(Il2CppClass*, il2cpp_type_get_class_or_element_class, (Il2CppType* type));
DO_API(char*, il2cpp_type_get_name, (Il2CppType* type));
DO_API(bool, il2cpp_type_is_byref, (Il2CppType* type));
DO_API(uint32_t, il2cpp_type_get_attrs, (Il2CppType* type));
DO_API(bool, il2cpp_type_equals, (Il2CppType* type, Il2CppType* otherType));
DO_API(char*, il2cpp_type_get_assembly_qualified_name, (Il2CppType* type));
DO_API(bool, il2cpp_type_is_static, (Il2CppType* type));

// Class
DO_API(Il2CppObject*, il2cpp_value_box, (Il2CppClass* klass, void* data));
DO_API(Il2CppObject*, il2cpp_object_new, (Il2CppClass* klass));
DO_API(Il2CppType*, il2cpp_class_enum_basetype, (Il2CppClass * klass));
DO_API(bool, il2cpp_class_is_generic, (Il2CppClass * klass));
DO_API(bool, il2cpp_class_is_inflated, (Il2CppClass * klass));
DO_API(bool, il2cpp_class_is_assignable_from, (Il2CppClass * klass, Il2CppClass * oklass));
DO_API(bool, il2cpp_class_is_subclass_of, (Il2CppClass * klass, Il2CppClass * klassc, bool check_interfaces));
DO_API(bool, il2cpp_class_has_parent, (Il2CppClass * klass, Il2CppClass * klassc));
DO_API(Il2CppClass*, il2cpp_class_get_element_class, (Il2CppClass * klass));
DO_API(Il2CppField*, il2cpp_class_get_fields, (Il2CppClass * klass, void** iter));
DO_API(Il2CppClass*, il2cpp_class_get_nested_types, (Il2CppClass * klass, void** iter));
DO_API(Il2CppClass*, il2cpp_class_get_interfaces, (Il2CppClass * klass, void** iter));
DO_API(Il2CppField*, il2cpp_class_get_field_from_name, (Il2CppClass * klass, const char *name));
DO_API(Il2CppMethod*, il2cpp_class_get_methods, (Il2CppClass * klass, void** iter));
DO_API(Il2CppMethod*, il2cpp_class_get_method_from_name, (Il2CppClass * klass, const char* name, int argsCount));
DO_API(const char*, il2cpp_class_get_name, (Il2CppClass * klass));
DO_API(const char*, il2cpp_class_get_namespace, (Il2CppClass * klass));
DO_API(Il2CppClass*, il2cpp_class_get_parent, (Il2CppClass * klass));
DO_API(Il2CppClass*, il2cpp_class_get_declaring_type, (Il2CppClass * klass));
DO_API(int32_t, il2cpp_class_instance_size, (Il2CppClass * klass));
DO_API(size_t, il2cpp_class_num_fields, (Il2CppClass * enumKlass));
DO_API(bool, il2cpp_class_is_valuetype, (Il2CppClass * klass));
DO_API(int32_t, il2cpp_class_value_size, (Il2CppClass * klass, uint32_t * align));
DO_API(bool, il2cpp_class_is_blittable, (Il2CppClass * klass));
DO_API(int, il2cpp_class_get_flags, (Il2CppClass * klass));
DO_API(bool, il2cpp_class_is_abstract, (Il2CppClass * klass));
DO_API(bool, il2cpp_class_is_interface, (Il2CppClass * klass));
DO_API(int, il2cpp_class_array_element_size, (Il2CppClass * klass));
DO_API(Il2CppType*, il2cpp_class_get_type, (Il2CppClass * klass));
DO_API(uint32_t, il2cpp_class_get_type_token, (Il2CppClass * klass));
DO_API(bool, il2cpp_class_has_attribute, (Il2CppClass * klass, Il2CppClass * attr_class));
DO_API(bool, il2cpp_class_has_references, (Il2CppClass * klass));
DO_API(bool, il2cpp_class_is_enum, (Il2CppClass * klass));
DO_API(Il2CppImage*, il2cpp_class_get_image, (Il2CppClass * klass));
DO_API(const char*, il2cpp_class_get_assemblyname, (Il2CppClass * klass));
DO_API(int, il2cpp_class_get_rank, (Il2CppClass * klass));

// liveness
DO_API(void*, il2cpp_unity_liveness_allocate_struct, (Il2CppClass * filter, int max_object_count, il2cpp_register_object_callback callback, void* userdata, il2cpp_liveness_reallocate_callback reallocate));
DO_API(void, il2cpp_unity_liveness_calculation_from_root, (Il2CppObject * root, void* state));
DO_API(void, il2cpp_unity_liveness_calculation_from_statics, (void* state));
DO_API(void, il2cpp_unity_liveness_finalize, (void* state));
DO_API(void, il2cpp_unity_liveness_free_struct, (void* state));


// Memory information
DO_API(ManagedMemorySnapshot*, il2cpp_capture_memory_snapshot, ());
DO_API(void, il2cpp_free_captured_memory_snapshot, (ManagedMemorySnapshot * snapshot));

DO_API(void, il2cpp_set_memory_callbacks, (Il2CppMemoryCallbacks * callbacks));
