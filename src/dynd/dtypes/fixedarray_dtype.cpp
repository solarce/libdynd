//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/dtypes/fixedarray_dtype.hpp>
#include <dynd/dtypes/strided_array_dtype.hpp>
#include <dynd/dtypes/dtype_alignment.hpp>
#include <dynd/shape_tools.hpp>
#include <dynd/shortvector.hpp>
#include <dynd/exceptions.hpp>
#include <dynd/gfunc/callable.hpp>

using namespace std;
using namespace dynd;

fixedarray_dtype::fixedarray_dtype(const dtype& element_dtype, size_t dimension_size)
    : m_element_dtype(element_dtype), m_dimension_size(dimension_size)
{
    size_t child_element_size = element_dtype.get_element_size();
    if (child_element_size == 0) {
        stringstream ss;
        ss << "Cannot create fixedarray dtype with element type " << element_dtype;
        ss << ", as it does not have a fixed size";
        throw runtime_error(ss.str());
    }
    m_stride = m_dimension_size > 1 ? element_dtype.get_element_size() : 0;
    m_element_size = m_dimension_size > 1 ? m_stride * m_dimension_size : m_dimension_size * child_element_size;

    create_ndobject_properties();
}

fixedarray_dtype::fixedarray_dtype(const dtype& element_dtype, size_t dimension_size, intptr_t stride)
    : m_element_dtype(element_dtype), m_stride(stride), m_dimension_size(dimension_size)
{
    size_t child_element_size = element_dtype.get_element_size();
    if (child_element_size == 0) {
        stringstream ss;
        ss << "Cannot create fixedarray dtype with element type " << element_dtype;
        ss << ", as it does not have a fixed size";
        throw runtime_error(ss.str());
    }
    if (dimension_size <= 1 && stride != 0) {
        stringstream ss;
        ss << "Cannot create fixedarray dtype with size " << dimension_size;
        ss << " and stride " << stride << ", as the stride must be zero when the dimension size is 1";
        throw runtime_error(ss.str());
    }
    if (dimension_size > 1 && stride == 0) {
        stringstream ss;
        ss << "Cannot create fixedarray dtype with size " << dimension_size;
        ss << " and stride 0, as the stride must be non-zero when the dimension size is > 1";
        throw runtime_error(ss.str());
    }
    m_element_size = stride ? stride * m_dimension_size : dimension_size * child_element_size;

    create_ndobject_properties();
}

void fixedarray_dtype::print_element(std::ostream& o, const char *metadata, const char *data) const
{
    size_t stride = m_stride;
    o << "[";
    for (size_t i = 0, i_end = m_dimension_size; i != i_end; ++i, data += stride) {
        m_element_dtype.print_element(o, metadata, data);
        if (i != i_end - 1) {
            o << ", ";
        }
    }
    o << "]";
}

void fixedarray_dtype::print_dtype(std::ostream& o) const
{
    o << "fixedarray<";
    o << m_dimension_size;
    if ((size_t)m_stride != m_element_dtype.get_element_size()) {
        o << ", stride=" << m_stride;
    }
    o << ", " << m_element_dtype;
    o << ">";
}

bool fixedarray_dtype::is_scalar() const
{
    return false;
}

bool fixedarray_dtype::is_expression() const
{
    return m_element_dtype.is_expression();
}

dtype fixedarray_dtype::with_transformed_scalar_types(dtype_transform_fn_t transform_fn, const void *extra) const
{
    dtype transformed_element_dtype = m_element_dtype.with_transformed_scalar_types(transform_fn, extra);
    // The transformed dtype may no longer have a fixed size, so check whether
    // we have to switch to the more flexible strided_array_dtype
    if (transformed_element_dtype.get_element_size() != 0) {
        return dtype(new fixedarray_dtype(transformed_element_dtype, m_dimension_size));
    } else {
        return dtype(new strided_array_dtype(transformed_element_dtype));
    }
}

dtype fixedarray_dtype::get_canonical_dtype() const
{
    dtype canonical_element_dtype = m_element_dtype.get_canonical_dtype();
    // The transformed dtype may no longer have a fixed size, so check whether
    // we have to switch to the more flexible strided_array_dtype
    if (canonical_element_dtype.get_element_size() != 0) {
        return dtype(new fixedarray_dtype(canonical_element_dtype, m_dimension_size));
    } else {
        return dtype(new strided_array_dtype(canonical_element_dtype));
    }
}

dtype fixedarray_dtype::apply_linear_index(int nindices, const irange *indices, int current_i, const dtype& root_dt) const
{
    if (nindices == 0) {
        return dtype(this, true);
    } else if (nindices == 1) {
        if (indices->step() == 0) {
            return m_element_dtype;
        } else {
            return dtype(this, true);
        }
    } else {
        if (indices->step() == 0) {
            return m_element_dtype.apply_linear_index(nindices-1, indices+1, current_i+1, root_dt);
        } else {
            return dtype(new strided_array_dtype(m_element_dtype.apply_linear_index(nindices-1, indices+1, current_i+1, root_dt)));
        }
    }
}

intptr_t fixedarray_dtype::apply_linear_index(int nindices, const irange *indices, char *data, const char *metadata,
                const dtype& result_dtype, char *out_metadata,
                memory_block_data *embedded_reference,
                int current_i, const dtype& root_dt) const
{
    if (nindices == 0) {
        // If there are no more indices, copy the rest verbatim
        if (m_element_dtype.extended()) {
            return m_element_dtype.extended()->apply_linear_index(0, NULL, data, metadata,
                            m_element_dtype, out_metadata, embedded_reference, current_i + 1, root_dt);
        }
        return 0;
    } else {
        bool remove_dimension;
        intptr_t start_index, index_stride, dimension_size;
        apply_single_linear_index(*indices, m_dimension_size, current_i, &root_dt, remove_dimension, start_index, index_stride, dimension_size);
        if (remove_dimension) {
            // Apply the strided offset and continue applying the index
            intptr_t offset = m_stride * start_index;
            if (m_element_dtype.extended()) {
                offset += m_element_dtype.extended()->apply_linear_index(nindices - 1, indices + 1,
                                data + offset, metadata,
                                result_dtype, out_metadata, embedded_reference, current_i + 1, root_dt);
            }
            return offset;
        } else {
            strided_array_dtype_metadata *out_md = reinterpret_cast<strided_array_dtype_metadata *>(out_metadata);
            // Produce the new offset data, stride, and size for the resulting array,
            // which is now a strided_array instead of a fixedarray
            intptr_t offset = m_stride * start_index;
            out_md->stride = m_stride * index_stride;
            out_md->size = dimension_size;
            if (m_element_dtype.extended()) {
                const fixedarray_dtype *result_edtype = static_cast<const fixedarray_dtype *>(result_dtype.extended());
                offset += m_element_dtype.extended()->apply_linear_index(nindices - 1, indices + 1,
                                data + offset, metadata,
                                result_edtype->m_element_dtype, out_metadata + sizeof(strided_array_dtype_metadata),
                                embedded_reference, current_i + 1, root_dt);
            }
            return offset;
        }
    }
}

dtype fixedarray_dtype::at(intptr_t i0, const char **DYND_UNUSED(inout_metadata), const char **inout_data) const
{
    // Bounds-checking of the index
    i0 = apply_single_index(i0, m_dimension_size, NULL);
    // The fixedarray dtype has no metadata
    // If requested, modify the data
    if (inout_data) {
        *inout_data += i0 * m_stride;
    }
    return m_element_dtype;
}

int fixedarray_dtype::get_uniform_ndim() const
{
    return 1 + m_element_dtype.get_uniform_ndim();
}

dtype fixedarray_dtype::get_dtype_at_dimension(char **inout_metadata, int i, int total_ndim) const
{
    if (i == 0) {
        return dtype(this, true);
    } else {
        return m_element_dtype.get_dtype_at_dimension(inout_metadata, i - 1, total_ndim + 1);
    }
}

intptr_t fixedarray_dtype::get_dim_size(const char *DYND_UNUSED(data), const char *DYND_UNUSED(metadata)) const
{
    return m_dimension_size;
}

void fixedarray_dtype::get_shape(int i, intptr_t *out_shape) const
{
    out_shape[i] = m_dimension_size;

    // Process the later shape values
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->get_shape(i+1, out_shape);
    }
}

void fixedarray_dtype::get_shape(int i, intptr_t *out_shape, const char *metadata) const
{
    out_shape[i] = m_dimension_size;

    // Process the later shape values
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->get_shape(i+1, out_shape, metadata);
    }
}

void fixedarray_dtype::get_strides(int i, intptr_t *out_strides, const char *metadata) const
{
    out_strides[i] = m_stride;

    // Process the later shape values
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->get_strides(i+1, out_strides, metadata);
    }
}

intptr_t fixedarray_dtype::get_representative_stride(const char *DYND_UNUSED(metadata)) const
{
    return m_stride;
}

bool fixedarray_dtype::is_lossless_assignment(const dtype& dst_dt, const dtype& src_dt) const
{
    if (dst_dt.extended() == this) {
        if (src_dt.extended() == this) {
            return true;
        } else if (src_dt.get_type_id() == fixedarray_type_id) {
            return *dst_dt.extended() == *src_dt.extended();
        }
    }

    return false;
}

void fixedarray_dtype::get_single_compare_kernel(single_compare_kernel_instance& DYND_UNUSED(out_kernel)) const
{
    throw runtime_error("fixedarray_dtype::get_single_compare_kernel is unimplemented"); 
}

void fixedarray_dtype::get_dtype_assignment_kernel(const dtype& DYND_UNUSED(dst_dt), const dtype& DYND_UNUSED(src_dt),
                assign_error_mode DYND_UNUSED(errmode),
                unary_specialization_kernel_instance& DYND_UNUSED(out_kernel)) const
{
    throw runtime_error("fixedarray_dtype::get_dtype_assignment_kernel is unimplemented"); 
}

bool fixedarray_dtype::operator==(const extended_dtype& rhs) const
{
    if (this == &rhs) {
        return true;
    } else if (rhs.get_type_id() != fixedarray_type_id) {
        return false;
    } else {
        const fixedarray_dtype *dt = static_cast<const fixedarray_dtype*>(&rhs);
        return m_element_dtype == dt->m_element_dtype &&
                m_dimension_size == dt->m_dimension_size &&
                m_stride == dt->m_stride;
    }
}

void fixedarray_dtype::metadata_default_construct(char *metadata, int ndim, const intptr_t* shape) const
{
    // Validate that the shape is ok
    if (ndim > 0) {
        if (shape[0] >= 0 && (size_t)shape[0] != m_dimension_size) {
            stringstream ss;
            ss << "Cannot construct dynd object of dtype " << dtype(this, true);
            ss << " with dimension size " << shape[0] << ", the size must be " << m_dimension_size;
            throw runtime_error(ss.str());
        }
    }

    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->metadata_default_construct(metadata, ndim-1, shape+1);
    }
}

void fixedarray_dtype::metadata_copy_construct(char *dst_metadata, const char *src_metadata, memory_block_data *embedded_reference) const
{
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->metadata_copy_construct(dst_metadata, src_metadata, embedded_reference);
    }
}

void fixedarray_dtype::metadata_destruct(char *metadata) const
{
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->metadata_destruct(metadata);
    }
}

void fixedarray_dtype::metadata_debug_print(const char *metadata, std::ostream& o, const std::string& indent) const
{
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->metadata_debug_print(metadata, o, indent);
    }
}

size_t fixedarray_dtype::get_iterdata_size(int ndim) const
{
    if (ndim == 0) {
        return 0;
    } else if (ndim == 1) {
        return sizeof(fixedarray_dtype_iterdata);
    } else {
        return m_element_dtype.get_iterdata_size(ndim - 1) + sizeof(fixedarray_dtype_iterdata);
    }
}

// Does one iterator increment for this dtype
static char *iterdata_incr(iterdata_common *iterdata, int level)
{
    fixedarray_dtype_iterdata *id = reinterpret_cast<fixedarray_dtype_iterdata *>(iterdata);
    if (level == 0) {
        id->data += id->stride;
        return id->data;
    } else {
        id->data = (id + 1)->common.incr(&(id + 1)->common, level - 1);
        return id->data;
    }
}

static char *iterdata_reset(iterdata_common *iterdata, char *data, int ndim)
{
    fixedarray_dtype_iterdata *id = reinterpret_cast<fixedarray_dtype_iterdata *>(iterdata);
    if (ndim == 1) {
        id->data = data;
        return data;
    } else {
        id->data = (id + 1)->common.reset(&(id + 1)->common, data, ndim - 1);
        return id->data;
    }
}

size_t fixedarray_dtype::iterdata_construct(iterdata_common *iterdata, const char **inout_metadata, int ndim, const intptr_t* shape, dtype& out_uniform_dtype) const
{
    size_t inner_size = 0;
    if (ndim > 1) {
        // Place any inner iterdata earlier than the outer iterdata
        inner_size = m_element_dtype.extended()->iterdata_construct(iterdata, inout_metadata,
                        ndim - 1, shape + 1, out_uniform_dtype);
        iterdata = reinterpret_cast<iterdata_common *>(reinterpret_cast<char *>(iterdata) + inner_size);
    } else {
        out_uniform_dtype = m_element_dtype;
    }

    if (m_dimension_size != 1 && (size_t)shape[0] != m_dimension_size) {
        stringstream ss;
        ss << "Cannot construct dynd iterator of dtype " << dtype(this, true);
        ss << " with dimension size " << shape[0] << ", the size must be " << m_dimension_size;
        throw runtime_error(ss.str());
    }

    fixedarray_dtype_iterdata *id = reinterpret_cast<fixedarray_dtype_iterdata *>(iterdata);

    id->common.incr = &iterdata_incr;
    id->common.reset = &iterdata_reset;
    id->data = NULL;
    id->stride = m_stride;

    return inner_size + sizeof(fixedarray_dtype_iterdata);
}

size_t fixedarray_dtype::iterdata_destruct(iterdata_common *iterdata, int ndim) const
{
    size_t inner_size = 0;
    if (ndim > 1) {
        inner_size = m_element_dtype.extended()->iterdata_destruct(iterdata, ndim - 1);
    }
    // No dynamic data to free
    return inner_size + sizeof(fixedarray_dtype_iterdata);
}

void fixedarray_dtype::foreach_leading(char *data, const char *metadata, foreach_fn_t callback, void *callback_data) const
{
    intptr_t stride = m_stride;
    for (intptr_t i = 0, i_end = m_dimension_size; i < i_end; ++i, data += stride) {
        callback(m_element_dtype, data, metadata, callback_data);
    }
}

void fixedarray_dtype::reorder_default_constructed_strides(char *DYND_UNUSED(dst_metadata),
                const dtype& DYND_UNUSED(src_dtype), const char *DYND_UNUSED(src_metadata)) const
{
    // Because everything contained in the fixedarray must have fixed size, it can't
    // be reordered. This makes this function a NOP
}

dtype dynd::make_fixedarray_dtype(const dtype& uniform_dtype, int ndim, const intptr_t *shape, const int *axis_perm)
{
    if (axis_perm == NULL) {
        // Build a C-order fixed array dtype
        dtype result = uniform_dtype;
        for (int i = ndim-1; i >= 0; --i) {
            result = make_fixedarray_dtype(result, shape[i]);
        }
        return result;
    } else {
        // Create strides with the axis permutation
        dimvector strides(ndim);
        intptr_t stride = uniform_dtype.get_element_size();
        for (int i = 0; i < ndim; ++i) {
            int i_perm = axis_perm[i];
            size_t dim_size = shape[i_perm];
            strides[i_perm] = dim_size > 1 ? stride : 0;
            stride *= dim_size;
        }
        // Build the fixed array dtype
        dtype result = uniform_dtype;
        for (int i = ndim-1; i >= 0; --i) {
            result = make_fixedarray_dtype(result, shape[i], strides[i]);
        }
        return result;
    }
}

void fixedarray_dtype::create_ndobject_properties()
{
    // TODO: This code is shared with all other uniform array dtypes, refactor it to a common place
    // Copy ndobject properties from the first non-uniform dimension
    if (m_element_dtype.extended()) {
        int ndim = m_element_dtype.get_uniform_ndim();
        int count = 0;
        const std::pair<std::string, gfunc::callable> *properties;
        if (ndim == 0) {
            m_element_dtype.extended()->get_dynamic_ndobject_properties(&properties, &count);
        } else {
            dtype dt = m_element_dtype;
            for (int i = 0; i < ndim; ++i) {
                dt = dt.at(0);
            }
            if (dt.extended()) {
                dt.extended()->get_dynamic_ndobject_properties(&properties, &count);
            }
        }
        m_ndobject_properties.resize(count);
        for (int i = 0; i < count; ++i) {
            m_ndobject_properties[i] = properties[i];
        }
    }
}

void fixedarray_dtype::get_dynamic_ndobject_properties(const std::pair<std::string, gfunc::callable> **out_properties, int *out_count) const
{
    *out_properties = m_ndobject_properties.empty() ? NULL : &m_ndobject_properties[0];
    *out_count = (int)m_ndobject_properties.size();
}