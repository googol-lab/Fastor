#ifndef ABSTRACT_TENSOR_FUNCTIONS_H
#define ABSTRACT_TENSOR_FUNCTIONS_H

#include "tensor/Tensor.h"
#include "meta/tensor_post_meta.h"

namespace Fastor {


template<class Derived, size_t DIMS>
FASTOR_INLINE typename tensor_type_finder<Derived>::type evaluate(const AbstractTensor<Derived,DIMS> &_src) {
    typename tensor_type_finder<Derived>::type out = _src;
    return out;
}


// These are the set of functions work on any expression without themselves being a
// Fastor expression. Note that all the mathematical functions (sin, cos etc) are
// Fastor expressions


template<class Derived, size_t DIMS>
FASTOR_INLINE typename Derived::scalar_type sum(const AbstractTensor<Derived,DIMS> &_src) {
    using T = typename Derived::scalar_type;
    using V = SIMDVector<T,DEFAULT_ABI>;
    const Derived &src = _src.self();
    FASTOR_INDEX i;
    T _scal=0; V _vec(_scal);
    for (i = 0; i < ROUND_DOWN(src.size(),V::Size); i+=V::Size) {
        _vec += src.template eval<T>(i);
    }
    for (; i < src.size(); ++i) {
        _scal += src.template eval_s<T>(i);
    }
    return _vec.sum() + _scal;
}

template<class Derived, size_t DIMS>
FASTOR_INLINE typename Derived::scalar_type product(const AbstractTensor<Derived,DIMS> &_src) {
    using T = typename Derived::scalar_type;
    using V = SIMDVector<T,DEFAULT_ABI>;
    const Derived &src = _src.self();
    FASTOR_INDEX i;
    T _scal=0; V _vec(_scal);
    for (i = 0; i < ROUND_DOWN(src.size(),V::Size); i+=V::Size) {
        _vec *= src.template eval<T>(i);
    }
    for (; i < src.size(); ++i) {
        _scal *= src.template eval_s<T>(i);
    }
    return _vec.product() * _scal;
}




template<class Derived, size_t DIMS>
FASTOR_INLINE typename Derived::scalar_type norm(const AbstractTensor<Derived,DIMS> &_src) {
    using T = typename Derived::scalar_type;
    using V = SIMDVector<T,DEFAULT_ABI>;
    const Derived &src = _src.self();
    FASTOR_INDEX i;
    T _scal=0; V _vec(_scal);
    for (i = 0; i < ROUND_DOWN(src.size(),V::Size); i+=V::Size) {
        // Evaluate the expression once
        auto eval_vec = src.template eval<T>(i);
#ifdef __FMA__
        _vec = fmadd(eval_vec,eval_vec,_vec);
#else
        _vec += eval_vec*eval_vec;
#endif
    }
    for (; i < src.size(); ++i) {
        // Evaluate the expression once
        auto eval_scal = src.template eval_s<T>(i);
        _scal += eval_scal*eval_scal;
    }
    return sqrts(_vec.sum() + _scal);
}


template<class Derived0, typename Derived1, size_t DIMS,
    typename std::enable_if<std::is_same<typename Derived0::scalar_type, typename Derived1::scalar_type>::value,bool>::type=0>
FASTOR_INLINE typename Derived0::scalar_type inner(const AbstractTensor<Derived0,DIMS> &_a, const AbstractTensor<Derived1,DIMS> &_b) {
    using T = typename Derived0::scalar_type;
    using V = SIMDVector<T,DEFAULT_ABI>;
    const Derived0 &srca = _a.self();
    const Derived1 &srcb = _b.self();
#ifdef NDEBUG
    FASTOR_ASSERT(srca.size()==srcb.size(), "EXPRESSION SIZE MISMATCH");
#endif
    FASTOR_INDEX i;
    T _scal=0; V _vec(_scal);
    for (i = 0; i < ROUND_DOWN(srca.size(),V::Size); i+=V::Size) {
#ifdef __FMA__
        _vec = fmadd(srca.template eval<T>(i),srcb.template eval<T>(i),_vec);
#else
        _vec += srca.template eval<T>(i)*srcb.template eval<T>(i);
#endif
    }
    for (; i < srca.size(); ++i) {
        // Evaluate the expression once
        _scal += srca.template eval_s<T>(i)*srcb.template eval_s<T>(i);
    }
    return _vec.sum() + _scal;
}



template<class Derived, size_t DIMS>
FASTOR_INLINE typename Derived::scalar_type trace(const AbstractTensor<Derived,DIMS> &_src) {
    using T = typename Derived::scalar_type;
    using tensor_type = typename tensor_type_finder<Derived>::type;
    constexpr std::array<size_t, DIMS> dims = LastMatrixExtracter<tensor_type,
        typename std_ext::make_index_sequence<DIMS>::type>::values;
    constexpr size_t M = dims[DIMS-2];
    constexpr size_t N = dims[DIMS-1];
    static_assert(DIMS==2,"TENSOR EXPRESSION SHOULD BE UNIFORM (SQUARE)");
    static_assert(M==N,"TENSOR EXPRESSION SHOULD BE TWO DIMENSIONAL");

    const Derived &src = _src.self();
    FASTOR_INDEX i;
    T _scal=0;
    for (i = 0; i < M; ++i) {
        _scal += src.template eval_s<T>(i*(N+1));
    }
    return _scal;
}


}


#endif // #ifndef TENSOR_FUNCTIONS_H