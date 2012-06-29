//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DND__STRIDED_NDARRAY_NODE_HPP_
#define _DND__STRIDED_NDARRAY_NODE_HPP_

#include <dnd/nodes/ndarray_node.hpp>

namespace dnd {

/**
 * NDArray node which holds a raw strided array.
 */
class strided_ndarray_node : public ndarray_node {
    /* The number of dimensions in the result array */
    int m_ndim;
    int m_access_flags;
    /* The shape of the result array */
    dimvector m_shape;
    /* The data type of this node's result */
    dtype m_dtype;
    char *m_originptr;
    dimvector m_strides;
    memory_block_ref m_memblock;

    // Non-copyable
    strided_ndarray_node(const strided_ndarray_node&);
    strided_ndarray_node& operator=(const strided_ndarray_node&);

public:
    /**
     * Creates a strided array node from the raw values. Does not validate them.
     */
    strided_ndarray_node(const dtype& dt, int ndim, const intptr_t *shape,
            const intptr_t *strides, char *originptr, int access_flags, const memory_block_ref& memblock);

    /**
     * Creates a strided array node from the raw values. Does not validate them.
     */
    strided_ndarray_node(const dtype& dt, int ndim, const intptr_t *shape,
            const intptr_t *strides, const char *originptr, int access_flags, const memory_block_ref& memblock);

#ifdef DND_RVALUE_REFS
    /**
     * Creates a strided array node from the raw values. Does not validate them.
     */
    strided_ndarray_node(const dtype& dt, int ndim, const intptr_t *shape,
            const intptr_t *strides, char *originptr, int access_flags, memory_block_ref&& memblock);

    /**
     * Creates a strided array node from the raw values. Does not validate them.
     */
    strided_ndarray_node(const dtype& dt, int ndim, const intptr_t *shape,
            const intptr_t *strides, const char *originptr, int access_flags, memory_block_ref&& memblock);
#endif

    /**
     * Constructs a strided array node with the given dtype, shape, and axis_perm (for memory layout)
     */
    strided_ndarray_node(const dtype& dt, int ndim, const intptr_t *shape, const int *axis_perm);

    virtual ~strided_ndarray_node() {
    }

    ndarray_node_category get_category() const
    {
        return strided_array_node_category;
    }

    const dtype& get_dtype() const {
        return m_dtype;
    }

    int get_ndim() const
    {
        return m_ndim;
    }

    const intptr_t *get_shape() const
    {
        return m_shape.get();
    }

    const intptr_t *get_strides() const {
        return m_strides.get();
    }

    uint32_t get_access_flags() const
    {
        return m_access_flags;
    }
    
    char *get_readwrite_originptr() const {
        if (m_access_flags & write_access_flag) {
            return m_originptr;
        } else {
            throw std::runtime_error("dnd::ndarray node is not writeable");
        }
    }

    const char *get_readonly_originptr() const {
        return m_originptr;
    }

    memory_block_ref get_memory_block() const {
        return m_memblock;
    }

    ndarray_node_ref as_dtype(const dtype& dt,
                        assign_error_mode errmode, bool allow_in_place);

    ndarray_node_ref apply_linear_index(
                    int ndim, const bool *remove_axis,
                    const intptr_t *start_index, const intptr_t *index_strides,
                    const intptr_t *shape,
                    bool allow_in_place);

    const char *node_name() const {
        return "strided_array";
    }

    void debug_dump_extra(std::ostream& o, const std::string& indent) const;
};

} // namespace dnd

#endif // _DND__STRIDED_NDARRAY_NODE_HPP_
