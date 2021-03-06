//
// Copyright (C) 2011-15 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <vector>
#include <string>

#include <dynd/type.hpp>
#include <dynd/types/tuple_type.hpp>
#include <dynd/types/pointer_type.hpp>
#include <dynd/types/type_type.hpp>
#include <dynd/memblock/memory_block.hpp>

namespace dynd {
namespace ndt {

  class DYND_API struct_type : public tuple_type {
    nd::array m_field_names;
    std::map<std::string, nd::callable> m_array_properties;

    void create_array_properties();

    // Special constructor to break the property parameter cycle in
    // create_array_properties
    struct_type(int, int);

  protected:
    uintptr_t *get_arrmeta_data_offsets(char *arrmeta) const { return reinterpret_cast<uintptr_t *>(arrmeta); }

  public:
    struct_type(const nd::array &field_names, const nd::array &field_types, bool variadic);

    virtual ~struct_type();

    /** The array of the field names */
    const nd::array &get_field_names() const { return m_field_names; }
    const string &get_field_name_raw(intptr_t i) const { return unchecked_fixed_dim_get<string>(m_field_names, i); }
    const std::string get_field_name(intptr_t i) const
    {
      const string &std(get_field_name_raw(i));
      return std::string(std.begin(), std.end());
    }

    /**
     * Gets the field index for the given name. Returns -1 if
     * the struct doesn't have a field of the given name.
     *
     * \param field_name  The name of the field.
     *
     * \returns  The field index, or -1 if there is not field
     *           of the given name.
     */
    inline intptr_t get_field_index(const std::string &field_name) const
    {
      return get_field_index(field_name.data(), field_name.data() + field_name.size());
    }
    intptr_t get_field_index(const char *field_name_begin, const char *field_name_end) const;

    inline const uintptr_t *get_data_offsets(const char *arrmeta) const
    {
      return reinterpret_cast<const uintptr_t *>(arrmeta);
    }

    void print_type(std::ostream &o) const;

    void transform_child_types(type_transform_fn_t transform_fn, intptr_t arrmeta_offset, void *extra,
                               type &out_transformed_tp, bool &out_was_transformed) const;
    type get_canonical_type() const;

    type at_single(intptr_t i0, const char **inout_arrmeta, const char **inout_data) const;

    bool is_lossless_assignment(const type &dst_tp, const type &src_tp) const;

    bool operator==(const base_type &rhs) const;

    void arrmeta_debug_print(const char *arrmeta, std::ostream &o, const std::string &indent) const;

    type apply_linear_index(intptr_t nindices, const irange *indices, size_t current_i, const type &root_tp,
                            bool leading_dimension) const;
    intptr_t apply_linear_index(intptr_t nindices, const irange *indices, const char *arrmeta, const type &result_tp,
                                char *out_arrmeta, const intrusive_ptr<memory_block_data> &embedded_reference,
                                size_t current_i, const type &root_tp, bool leading_dimension, char **inout_data,
                                intrusive_ptr<memory_block_data> &inout_dataref) const;

    size_t make_comparison_kernel(void *ckb, intptr_t ckb_offset, const type &src0_dt, const char *src0_arrmeta,
                                  const type &src1_dt, const char *src1_arrmeta, comparison_type_t comptype,
                                  const eval::eval_context *ectx) const;

    void get_dynamic_type_properties(std::map<std::string, nd::callable> &properties) const;
    void get_dynamic_array_properties(std::map<std::string, nd::callable> &properties) const;

    virtual bool match(const char *arrmeta, const type &candidate_tp, const char *candidate_arrmeta,
                       std::map<std::string, type> &tp_vars) const;

    size_t get_elwise_property_index(const std::string &property_name) const;
    type get_elwise_property_type(size_t elwise_property_index, bool &out_readable, bool &out_writable) const;
    size_t make_elwise_property_getter_kernel(void *ckb, intptr_t ckb_offset, const char *dst_arrmeta,
                                              const char *src_arrmeta, size_t src_elwise_property_index,
                                              kernel_request_t kernreq, const eval::eval_context *ectx) const;
    size_t make_elwise_property_setter_kernel(void *ckb, intptr_t ckb_offset, const char *dst_arrmeta,
                                              size_t dst_elwise_property_index, const char *src_arrmeta,
                                              kernel_request_t kernreq, const eval::eval_context *ectx) const;

    /** Makes a struct type with the specified fields */
    static type make(const nd::array &field_names, const nd::array &field_types, bool variadic = false)
    {
      return type(new struct_type(field_names, field_types, variadic), false);
    }

    /** Makes an empty struct type */
    static type make(bool variadic = false)
    {
      return make(nd::empty(0, string_type::make()), nd::empty(0, make_type<type_type>()), variadic);
    }
  };

} // namespace dynd::ndt

/**
 * Concatenates the fields of two structs together into one.
 */
DYND_API nd::array struct_concat(nd::array lhs, nd::array rhs);

} // namespace dynd
