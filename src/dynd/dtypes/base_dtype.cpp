//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/dtype.hpp>
#include <dynd/gfunc/callable.hpp>

using namespace std;
using namespace dynd;

// Default destructor for the extended dtype does nothing
base_dtype::~base_dtype()
{
}

bool base_dtype::is_scalar() const
{
    return true;
}

bool base_dtype::is_uniform_dim() const
{
    return false;
}

bool base_dtype::is_expression() const
{
    return false;
}

void base_dtype::transform_child_dtypes(dtype_transform_fn_t DYND_UNUSED(transform_fn), const void *DYND_UNUSED(extra),
                dtype& out_transformed_dtype, bool& DYND_UNUSED(out_was_transformed)) const
{
    // Default to behavior with no child dtypes
    out_transformed_dtype = dtype(this, true);
}

dtype base_dtype::get_canonical_dtype() const
{
    // Default to no transformation of the dtype
    return dtype(this, true);
}

dtype base_dtype::apply_linear_index(int nindices, const irange *DYND_UNUSED(indices),
                int current_i, const dtype& DYND_UNUSED(root_dt)) const
{
    // Default to scalar behavior
    if (nindices == 0) {
        return dtype(this, true);
    } else {
        throw too_many_indices(dtype(this, true), current_i + nindices, current_i);
    }
}

intptr_t base_dtype::apply_linear_index(int nindices, const irange *DYND_UNUSED(indices), char *DYND_UNUSED(data), const char *DYND_UNUSED(metadata),
                const dtype& DYND_UNUSED(result_dtype), char *DYND_UNUSED(out_metadata),
                memory_block_data *DYND_UNUSED(embedded_reference),
                int current_i, const dtype& DYND_UNUSED(root_dt)) const
{
    // Default to scalar behavior
    if (nindices == 0) {
        return 0;
    } else {
        throw too_many_indices(dtype(this, true), current_i + nindices, current_i);
    }
}

dtype base_dtype::at(intptr_t DYND_UNUSED(i0), const char **DYND_UNUSED(inout_metadata),
                const char **DYND_UNUSED(inout_data)) const
{
    // Default to scalar behavior
    throw too_many_indices(dtype(this, true), 1, 0);
}

dtype base_dtype::get_dtype_at_dimension(char **DYND_UNUSED(inout_metadata), size_t i, size_t total_ndim) const
{
    // Default to heterogeneous dimension/scalar behavior
    if (i == 0) {
        return dtype(this, true);
    } else {
        throw too_many_indices(dtype(this, true), total_ndim + i, total_ndim);
    }
}

intptr_t base_dtype::get_dim_size(const char *DYND_UNUSED(data), const char *DYND_UNUSED(metadata)) const
{
    // Default to scalar behavior
    stringstream ss;
    ss << "Cannot get the leading dimension size of ndobject with scalar dtype " << dtype(this, true);
    throw std::runtime_error(ss.str());
}

void base_dtype::get_shape(size_t DYND_UNUSED(i), intptr_t *DYND_UNUSED(out_shape)) const
{
    // Default to scalar behavior
}

void base_dtype::get_shape(size_t DYND_UNUSED(i), intptr_t *DYND_UNUSED(out_shape),
                    const char *DYND_UNUSED(metadata)) const
{
    // Default to scalar behavior
}

void base_dtype::get_strides(size_t DYND_UNUSED(i), intptr_t *DYND_UNUSED(out_strides),
                    const char *DYND_UNUSED(metadata)) const
{
    // Default to scalar behavior
}

intptr_t base_dtype::get_representative_stride(const char *DYND_UNUSED(metadata)) const
{
    // Default to scalar behavior
    stringstream ss;
    ss << "Cannot get a representative stride for scalar dtype " << dtype(this, true);
    throw std::runtime_error(ss.str());
}

size_t base_dtype::get_default_data_size(int DYND_UNUSED(ndim), const intptr_t *DYND_UNUSED(shape)) const
{
    return get_data_size();
}

// TODO: Make this a pure virtual function eventually
size_t base_dtype::get_metadata_size() const
{
    stringstream ss;
    ss << "TODO: get_metadata_size for " << dtype(this, true) << " is not implemented";
    throw std::runtime_error(ss.str());
}

// TODO: Make this a pure virtual function eventually
void base_dtype::metadata_default_construct(char *DYND_UNUSED(metadata),
                int DYND_UNUSED(ndim), const intptr_t* DYND_UNUSED(shape)) const
{
    stringstream ss;
    ss << "TODO: metadata_default_construct for " << dtype(this, true) << " is not implemented";
    throw std::runtime_error(ss.str());
}

void base_dtype::metadata_copy_construct(char *DYND_UNUSED(dst_metadata), const char *DYND_UNUSED(src_metadata), memory_block_data *DYND_UNUSED(embedded_reference)) const
{
    stringstream ss;
    ss << "TODO: metadata_copy_construct for " << dtype(this, true) << " is not implemented";
    throw std::runtime_error(ss.str());
}

void base_dtype::metadata_reset_buffers(char *DYND_UNUSED(metadata)) const
{
    // By default there are no buffers to reset
}

void base_dtype::metadata_finalize_buffers(char *DYND_UNUSED(metadata)) const
{
    // By default there are no buffers to finalize
}

// TODO: Make this a pure virtual function eventually
void base_dtype::metadata_destruct(char *DYND_UNUSED(metadata)) const
{
    stringstream ss;
    ss << "TODO: metadata_destruct for " << dtype(this, true) << " is not implemented";
    throw std::runtime_error(ss.str());
}

// TODO: Make this a pure virtual function eventually
void base_dtype::metadata_debug_print(const char *DYND_UNUSED(metadata), std::ostream& DYND_UNUSED(o), const std::string& DYND_UNUSED(indent)) const
{
    stringstream ss;
    ss << "TODO: metadata_debug_print for " << dtype(this, true) << " is not implemented";
    throw std::runtime_error(ss.str());
}

size_t base_dtype::get_iterdata_size(int DYND_UNUSED(ndim)) const
{
    stringstream ss;
    ss << "get_iterdata_size: dynd dtype " << dtype(this, true) << " is not uniformly iterable";
    throw std::runtime_error(ss.str());
}

size_t base_dtype::iterdata_construct(iterdata_common *DYND_UNUSED(iterdata), const char **DYND_UNUSED(inout_metadata),
                int DYND_UNUSED(ndim), const intptr_t* DYND_UNUSED(shape), dtype& DYND_UNUSED(out_uniform_dtype)) const
{
    stringstream ss;
    ss << "iterdata_default_construct: dynd dtype " << dtype(this, true) << " is not uniformly iterable";
    throw std::runtime_error(ss.str());
}

size_t base_dtype::iterdata_destruct(iterdata_common *DYND_UNUSED(iterdata), int DYND_UNUSED(ndim)) const
{
    stringstream ss;
    ss << "iterdata_destruct: dynd dtype " << dtype(this, true) << " is not uniformly iterable";
    throw std::runtime_error(ss.str());
}


void base_dtype::foreach_leading(char *DYND_UNUSED(data), const char *DYND_UNUSED(metadata),
                foreach_fn_t DYND_UNUSED(callback), void *DYND_UNUSED(callback_data)) const
{
    // Default to scalar behavior
    stringstream ss;
    ss << "dynd dtype " << dtype(this, true) << " is a scalar, foreach_leading cannot process";
    throw std::runtime_error(ss.str());
}

void base_dtype::reorder_default_constructed_strides(char *DYND_UNUSED(dst_metadata),
                const dtype& DYND_UNUSED(src_dtype), const char *DYND_UNUSED(src_metadata)) const
{
    // Default to scalar behavior, which is to do no modifications.
}

void base_dtype::get_nonuniform_ndobject_properties_and_functions(
                std::vector<std::pair<std::string, gfunc::callable> >& out_properties,
                std::vector<std::pair<std::string, gfunc::callable> >& out_functions) const
{
    // This copies properties from the first non-uniform dtype dimension to
    // the requested vectors. It is for use by uniform dtypes, which by convention
    // expose the properties from the first non-uniform dtypes, and possibly add
    // additional properties of their own.
    size_t ndim = get_undim();
    size_t properties_count = 0, functions_count = 0;
    const std::pair<std::string, gfunc::callable> *properties = NULL, *functions = NULL;
    if (ndim == 0) {
        get_dynamic_ndobject_properties(&properties, &properties_count);
        get_dynamic_ndobject_functions(&functions, &functions_count);
    } else {
        dtype dt = get_dtype_at_dimension(NULL, ndim);
        if (!dt.is_builtin()) {
            dt.extended()->get_dynamic_ndobject_properties(&properties, &properties_count);
            dt.extended()->get_dynamic_ndobject_functions(&functions, &functions_count);
        }
    }
    out_properties.resize(properties_count);
    for (size_t i = 0; i < properties_count; ++i) {
        out_properties[i] = properties[i];
    }
    out_functions.resize(functions_count);
    for (size_t i = 0; i < functions_count; ++i) {
        out_functions[i] = functions[i];
    }
}

void base_dtype::get_dynamic_dtype_properties(const std::pair<std::string, gfunc::callable> **out_properties, size_t *out_count) const
{
    // Default to no properties
    *out_properties = NULL;
    *out_count = 0;
}

void base_dtype::get_dynamic_dtype_functions(const std::pair<std::string, gfunc::callable> **out_functions, size_t *out_count) const
{
    // Default to no functions
    *out_functions = NULL;
    *out_count = 0;
}

void base_dtype::get_dynamic_ndobject_properties(const std::pair<std::string, gfunc::callable> **out_properties, size_t *out_count) const
{
    // Default to no properties
    *out_properties = NULL;
    *out_count = 0;
}

void base_dtype::get_dynamic_ndobject_functions(const std::pair<std::string, gfunc::callable> **out_functions, size_t *out_count) const
{
    // Default to no functions
    *out_functions = NULL;
    *out_count = 0;
}


void base_dtype::get_dtype_assignment_kernel(const dtype& dst_dt, const dtype& src_dt,
                assign_error_mode DYND_UNUSED(errmode),
                kernel_instance<unary_operation_pair_t>& DYND_UNUSED(out_kernel)) const
{
    stringstream ss;
    ss << "get_dtype_assignment_kernel has not been implemented for ";
    if (this == dst_dt.extended()) {
        ss << dst_dt;
    } else {
        ss << src_dt;
    }
    throw std::runtime_error(ss.str());
}

void base_dtype::get_single_compare_kernel(kernel_instance<compare_operations_t>& DYND_UNUSED(out_kernel)) const
{
        throw std::runtime_error("get_single_compare_kernel: this dtypes does not support comparisons");
}

