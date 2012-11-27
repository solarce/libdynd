//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <dynd/dtypes/strided_array_dtype.hpp>
#include <dynd/dtypes/dtype_alignment.hpp>
#include <dynd/shape_tools.hpp>
#include <dynd/exceptions.hpp>

using namespace std;
using namespace dynd;

dynd::strided_array_dtype::strided_array_dtype(const dtype& element_dtype)
    : m_element_dtype(element_dtype)
{
}

size_t dynd::strided_array_dtype::get_default_element_size(int ndim, const intptr_t *shape) const
{
    if (ndim == 0 || shape[0] < 0) {
        throw std::runtime_error("the strided_array dtype requires a shape be specified for default construction");
    }

    if (m_element_dtype.extended()) {
        return shape[0] * m_element_dtype.extended()->get_default_element_size(ndim-1, shape+1);
    } else {
        return shape[0] * m_element_dtype.element_size();
    }
}


void dynd::strided_array_dtype::print_element(std::ostream& o, const char *data, const char *metadata) const
{
    const strided_array_dtype_metadata *md = reinterpret_cast<const strided_array_dtype_metadata *>(metadata);
    size_t stride = md->stride;
    metadata += sizeof(strided_array_dtype_metadata);
    o << "[";
    for (size_t i = 0, i_end = md->size; i != i_end; ++i, data += stride) {
        m_element_dtype.print_element(o, data, metadata);
        if (i != i_end - 1) {
            o << ", ";
        }
    }
    o << "]";
}

void dynd::strided_array_dtype::print_dtype(std::ostream& o) const
{
    o << "strided_array<" << m_element_dtype << ">";
}

bool dynd::strided_array_dtype::is_uniform_dim() const
{
    return true;
}

bool dynd::strided_array_dtype::is_scalar(const char *DYND_UNUSED(data), const char *DYND_UNUSED(metadata)) const
{
    return false;
}

dtype dynd::strided_array_dtype::with_transformed_scalar_types(dtype_transform_fn_t transform_fn, const void *extra) const
{
    return dtype(new strided_array_dtype(m_element_dtype.with_transformed_scalar_types(transform_fn, extra)));
}

dtype dynd::strided_array_dtype::get_canonical_dtype() const
{
    return dtype(new strided_array_dtype(m_element_dtype.get_canonical_dtype()));
}

dtype dynd::strided_array_dtype::apply_linear_index(int nindices, const irange *indices, int current_i, const dtype& root_dt) const
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

intptr_t dynd::strided_array_dtype::apply_linear_index(int nindices, const irange *indices, char *data, const char *metadata,
                const dtype& result_dtype, char *out_metadata, int current_i, const dtype& root_dt) const
{
    const strided_array_dtype_metadata *md = reinterpret_cast<const strided_array_dtype_metadata *>(metadata);
    strided_array_dtype_metadata *out_md = reinterpret_cast<strided_array_dtype_metadata *>(out_metadata);
    if (nindices == 0) {
        // If there are no more indices, copy the rest verbatim
        *out_md = *md;
        if (m_element_dtype.extended()) {
            return m_element_dtype.extended()->apply_linear_index(0, NULL, data, metadata + sizeof(strided_array_dtype_metadata),
                            m_element_dtype, out_metadata + sizeof(strided_array_dtype_metadata), current_i + 1, root_dt);
        }
        return 0;
    } else {
        bool remove_dimension;
        intptr_t start_index, index_stride, dimension_size;
        apply_single_linear_index(*indices, md->size, current_i, &root_dt, remove_dimension, start_index, index_stride, dimension_size);
        if (remove_dimension) {
            // Apply the strided offset and continue applying the index
            intptr_t offset = md->stride * start_index;
            if (m_element_dtype.extended()) {
                offset += m_element_dtype.extended()->apply_linear_index(nindices - 1, indices + 1, data + offset,
                                metadata + sizeof(strided_array_dtype_metadata),
                                result_dtype, out_metadata, current_i + 1, root_dt);
            }
            return offset;
        } else {
            // Produce the new offset data, stride, and size for the resulting array
            intptr_t offset = md->stride * start_index;
            out_md->stride = md->stride * index_stride;
            out_md->size = dimension_size;
            if (m_element_dtype.extended()) {
                const strided_array_dtype *result_edtype = static_cast<const strided_array_dtype *>(result_dtype.extended());
                offset += m_element_dtype.extended()->apply_linear_index(nindices - 1, indices + 1, data + offset,
                                metadata + sizeof(strided_array_dtype_metadata),
                                result_edtype->m_element_dtype, out_metadata + sizeof(strided_array_dtype_metadata), current_i + 1, root_dt);
            }
            return offset;
        }
    }
}


int dynd::strided_array_dtype::get_uniform_ndim() const
{
    if (!m_element_dtype.extended()) {
        return 1;
    } else {
        return 1 + m_element_dtype.extended()->get_uniform_ndim();
    }
}

dtype dynd::strided_array_dtype::get_dtype_at_dimension(char **inout_metadata, int i, int total_ndim) const
{
    if (i == 0) {
        return dtype(this, true);
    } else {
        if (inout_metadata) {
            *inout_metadata += sizeof(strided_array_dtype_metadata);
        }
        return m_element_dtype.get_dtype_at_dimension(inout_metadata, i - 1, total_ndim + 1);
    }
}


void dynd::strided_array_dtype::get_shape(int i, intptr_t *out_shape) const
{
    // Adjust the current shape if necessary
    out_shape[i] = shape_signal_varying;

    // Process the later shape values
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->get_shape(i+1, out_shape);
    }
}

void ::strided_array_dtype::get_shape(int i, intptr_t *out_shape, const char *data, const char *metadata) const
{
    const strided_array_dtype_metadata *md = reinterpret_cast<const strided_array_dtype_metadata *>(metadata);

    out_shape[i] = md->size;

    // Process the later shape values
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->get_shape(i+1, out_shape, data, metadata + sizeof(strided_array_dtype_metadata));
    }
}

void ::strided_array_dtype::get_strides(int i, intptr_t *out_strides, const char *data, const char *metadata) const
{
    const strided_array_dtype_metadata *md = reinterpret_cast<const strided_array_dtype_metadata *>(metadata);

    out_strides[i] = md->stride;

    // Process the later shape values
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->get_strides(i+1, out_strides, data, metadata + sizeof(strided_array_dtype_metadata));
    }
}

bool dynd::strided_array_dtype::is_lossless_assignment(const dtype& dst_dt, const dtype& src_dt) const
{
    if (dst_dt.extended() == this) {
        if (src_dt.extended() == this) {
            return true;
        } else if (src_dt.type_id() == strided_array_type_id) {
            return *dst_dt.extended() == *src_dt.extended();
        }
    }

    return false;
}

void dynd::strided_array_dtype::get_single_compare_kernel(single_compare_kernel_instance& DYND_UNUSED(out_kernel)) const
{
    throw runtime_error("strided_array_dtype::get_single_compare_kernel is unimplemented"); 
}

void dynd::strided_array_dtype::get_dtype_assignment_kernel(const dtype& DYND_UNUSED(dst_dt), const dtype& DYND_UNUSED(src_dt),
                assign_error_mode DYND_UNUSED(errmode),
                unary_specialization_kernel_instance& DYND_UNUSED(out_kernel)) const
{
    throw runtime_error("strided_array_dtype::get_dtype_assignment_kernel is unimplemented"); 
}

bool dynd::strided_array_dtype::operator==(const extended_dtype& rhs) const
{
    if (this == &rhs) {
        return true;
    } else if (rhs.type_id() != strided_array_type_id) {
        return false;
    } else {
        const strided_array_dtype *dt = static_cast<const strided_array_dtype*>(&rhs);
        return m_element_dtype == dt->m_element_dtype;
    }
}

size_t dynd::strided_array_dtype::get_metadata_size() const
{
    size_t result = sizeof(strided_array_dtype_metadata);
    if (m_element_dtype.extended()) {
        result += m_element_dtype.extended()->get_metadata_size();
    }
    return result;
}

void dynd::strided_array_dtype::metadata_default_construct(char *metadata, int ndim, const intptr_t* shape) const
{
    // Validate that the shape is ok
    if (ndim == 0 || shape[0] < 0) {
        throw std::runtime_error("the strided_array dtype requires a shape be specified for default construction");
    }
    size_t element_size = m_element_dtype.extended() ? m_element_dtype.extended()->get_default_element_size(ndim-1, shape+1)
                                                     : m_element_dtype.element_size();

    strided_array_dtype_metadata *md = reinterpret_cast<strided_array_dtype_metadata *>(metadata);
    md->size = shape[0];
    if (shape[0] > 1) {
        md->stride = element_size;
    } else {
        md->stride = 0;
    }
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->metadata_default_construct(metadata + sizeof(strided_array_dtype_metadata), ndim-1, shape+1);
    }
}

void dynd::strided_array_dtype::metadata_copy_construct(char *dst_metadata, const char *src_metadata, memory_block_data *embedded_reference) const
{
    const strided_array_dtype_metadata *src_md = reinterpret_cast<const strided_array_dtype_metadata *>(src_metadata);
    strided_array_dtype_metadata *dst_md = reinterpret_cast<strided_array_dtype_metadata *>(dst_metadata);
    dst_md->size = src_md->size;
    dst_md->stride = src_md->stride;
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->metadata_copy_construct(dst_metadata + sizeof(strided_array_dtype_metadata),
                        src_metadata + sizeof(strided_array_dtype_metadata), embedded_reference);
    }
}

void dynd::strided_array_dtype::metadata_destruct(char *metadata) const
{
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->metadata_destruct(metadata + sizeof(strided_array_dtype_metadata));
    }
}

void dynd::strided_array_dtype::metadata_debug_dump(const char *metadata, std::ostream& o, const std::string& indent) const
{
    const strided_array_dtype_metadata *md = reinterpret_cast<const strided_array_dtype_metadata *>(metadata);
    o << indent << "strided_array_dtype metadata\n";
    o << indent << " stride: " << md->stride << "\n";
    o << indent << " size: " << md->size << "\n";
    if (m_element_dtype.extended()) {
        m_element_dtype.extended()->metadata_debug_dump(metadata + sizeof(strided_array_dtype_metadata), o, indent + " ");
    } else {
        o << indent << "  " << m_element_dtype << "\n";
    }
}

size_t strided_array_dtype::get_iterdata_size(int ndim) const
{
    if (ndim == 0) {
        return 0;
    } else if (ndim == 1) {
        return sizeof(strided_array_dtype_iterdata);
    } else {
        return m_element_dtype.get_iterdata_size(ndim - 1) + sizeof(strided_array_dtype_iterdata);
    }
}

// Does one iterator increment for this dtype
static char *iterdata_incr(iterdata_common *iterdata, int level)
{
    strided_array_dtype_iterdata *id = reinterpret_cast<strided_array_dtype_iterdata *>(iterdata);
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
    strided_array_dtype_iterdata *id = reinterpret_cast<strided_array_dtype_iterdata *>(iterdata);
    if (ndim == 1) {
        id->data = data;
        return data;
    } else {
        id->data = (id + 1)->common.reset(&(id + 1)->common, data, ndim - 1);
        return id->data;
    }
}

size_t strided_array_dtype::iterdata_construct(iterdata_common *iterdata, const char **inout_metadata, int ndim, const intptr_t* shape, dtype& out_uniform_dtype) const
{
    const strided_array_dtype_metadata *md = reinterpret_cast<const strided_array_dtype_metadata *>(*inout_metadata);
    size_t inner_size = 0;
    if (ndim > 1) {
        // Place any inner iterdata earlier than the outer iterdata
        *inout_metadata += sizeof(strided_array_dtype_metadata);
        inner_size = m_element_dtype.extended()->iterdata_construct(iterdata, inout_metadata,
                        ndim - 1, shape + 1, out_uniform_dtype);
        iterdata = reinterpret_cast<iterdata_common *>(reinterpret_cast<char *>(iterdata) + inner_size);
    } else {
        out_uniform_dtype = m_element_dtype;
    }

    strided_array_dtype_iterdata *id = reinterpret_cast<strided_array_dtype_iterdata *>(iterdata);

    id->common.incr = &iterdata_incr;
    id->common.reset = &iterdata_reset;
    id->data = NULL;
    id->stride = md->stride;

    return inner_size + sizeof(strided_array_dtype_iterdata);
}

size_t strided_array_dtype::iterdata_destruct(iterdata_common *iterdata, int ndim) const
{
    size_t inner_size = 0;
    if (ndim > 1) {
        inner_size = m_element_dtype.extended()->iterdata_destruct(iterdata, ndim - 1);
    }
    // No dynamic data to free
    return inner_size + sizeof(strided_array_dtype_iterdata);
}

void dynd::strided_array_dtype::foreach(int ndim, char *data, const char *metadata, foreach_fn_t callback, const void *callback_data) const
{
    if (ndim > 0) {
        const strided_array_dtype_metadata *md = reinterpret_cast<const strided_array_dtype_metadata *>(metadata);
        const extended_dtype *child_dtype = m_element_dtype.extended();
        const char *child_metadata = metadata + sizeof(strided_array_dtype_metadata);
        intptr_t stride = md->stride;
        for (intptr_t i = 0, i_end = md->size; i < i_end; ++i, data += stride) {
        
        }
    } else {
        callback(this, data, metadata, callback_data);
    }
}