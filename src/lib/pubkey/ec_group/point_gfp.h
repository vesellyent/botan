/*
* Point arithmetic on elliptic curves over GF(p)
*
* (C) 2007 Martin Doering, Christoph Ludwig, Falko Strenzke
*     2008-2011,2014,2015,2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_POINT_GFP_H_
#define BOTAN_POINT_GFP_H_

#include <botan/curve_gfp.h>
#include <botan/exceptn.h>
#include <vector>

namespace Botan {

/**
* This class represents one point on a curve of GF(p) using Jacobian coordinates
*/
class BOTAN_PUBLIC_API(2,0) PointGFp final
   {
   public:
      enum Compression_Type {
         UNCOMPRESSED = 0,
         COMPRESSED   = 1,
         HYBRID       = 2
      };

      enum { WORKSPACE_SIZE = 8 };

      /**
      * Construct an uninitialized PointGFp
      */
      PointGFp() = default;

      /**
      * Construct the zero point
      * @param curve The base curve
      */
      explicit PointGFp(const CurveGFp& curve);

      /**
      * Copy constructor
      */
      PointGFp(const PointGFp&) = default;

      /**
      * Move Constructor
      */
      PointGFp(PointGFp&& other)
         {
         this->swap(other);
         }

      /**
      * Standard Assignment
      */
      PointGFp& operator=(const PointGFp&) = default;

      /**
      * Move Assignment
      */
      PointGFp& operator=(PointGFp&& other)
         {
         if(this != &other)
            this->swap(other);
         return (*this);
         }

      /**
      * Return the zero (aka infinite) point associated with this curve
      */
      PointGFp zero() const { return PointGFp(m_curve); }

      const BigInt& get_x() const { return m_coord_x; }
      const BigInt& get_y() const { return m_coord_y; }
      const BigInt& get_z() const { return m_coord_z; }

      /**
      * Construct a point from its affine coordinates
      * Prefer EC_Group::point(x,y) for this operation.
      * @param curve the base curve
      * @param x affine x coordinate
      * @param y affine y coordinate
      */
      PointGFp(const CurveGFp& curve, const BigInt& x, const BigInt& y);

      /**
      * EC2OSP - elliptic curve to octet string primitive
      * @param format which format to encode using
      */
      std::vector<uint8_t> encode(PointGFp::Compression_Type format) const;

      /**
      * += Operator
      * @param rhs the PointGFp to add to the local value
      * @result resulting PointGFp
      */
      PointGFp& operator+=(const PointGFp& rhs);

      /**
      * -= Operator
      * @param rhs the PointGFp to subtract from the local value
      * @result resulting PointGFp
      */
      PointGFp& operator-=(const PointGFp& rhs);

      /**
      * *= Operator
      * @param scalar the PointGFp to multiply with *this
      * @warning this leaks the bitlength of the scalar
      * @result resulting PointGFp
      */
      PointGFp& operator*=(const BigInt& scalar);

      /**
      * Negate this point
      * @return *this
      */
      PointGFp& negate()
         {
         if(!is_zero())
            m_coord_y = m_curve.get_p() - m_coord_y;
         return *this;
         }

      /**
      * Return true if this point is in affine coordinates, ie Z=1
      */
      bool is_affine() const;

      /**
      * Equality operator
      */
      bool operator==(const PointGFp& other) const;

      /**
      * get affine x coordinate
      * @result affine x coordinate
      */
      BigInt get_affine_x(BN_Pool& pool) const;

      /**
      * get affine y coordinate
      * @result affine y coordinate
      */
      BigInt get_affine_y(BN_Pool& pool) const;

      /**
      * Force this point to affine coordinates
      */
      void force_affine(BN_Pool& pool);

      /**
      * Force all points on the list to affine coordinates
      */
      static void force_all_affine(std::vector<PointGFp>& points, BN_Pool& pool);

      /**
      * Is this the point at infinity?
      * @result true, if this point is at infinity, false otherwise.
      */
      bool is_zero() const { return m_coord_z.is_zero(); }

      /**
      * Checks whether the point is to be found on the underlying
      * curve; used to prevent fault attacks.
      * @return if the point is on the curve
      */
      bool on_the_curve(BN_Pool& pool) const;

      /**
      * swaps the states of *this and other, does not throw!
      * @param other the object to swap values with
      */
      void swap(PointGFp& other);

      /**
      * Randomize the point representation
      * The actual value (get_affine_x, get_affine_y) does not change
      */
      void randomize_repr(RandomNumberGenerator& rng);

      /**
      * Randomize the point representation
      * The actual value (get_affine_x, get_affine_y) does not change
      */
      void randomize_repr(RandomNumberGenerator& rng, secure_vector<word>& ws);

      /**
      * Point addition
      * @param other the point to add to *this
      * @param pool a BN_Pool
      */
      void add(const PointGFp& other, BN_Pool& pool)
         {
         BOTAN_ARG_CHECK(m_curve == other.m_curve, "cannot add points on different curves");

         const size_t p_words = m_curve.get_p_words();

         add(other.m_coord_x.data(), std::min(p_words, other.m_coord_x.size()),
             other.m_coord_y.data(), std::min(p_words, other.m_coord_y.size()),
             other.m_coord_z.data(), std::min(p_words, other.m_coord_z.size()),
             pool);
         }

      /**
      * Point addition. Array version.
      *
      * @param x_words the words of the x coordinate of the other point
      * @param x_size size of x_words
      * @param y_words the words of the y coordinate of the other point
      * @param y_size size of y_words
      * @param z_words the words of the z coordinate of the other point
      * @param z_size size of z_words
      * @param pool a BN_Pool
      */
      void add(const word x_words[], size_t x_size,
               const word y_words[], size_t y_size,
               const word z_words[], size_t z_size,
               BN_Pool& pool);

      /**
      * Point addition - mixed J+A
      * @param other affine point to add - assumed to be affine!
      * @param pool a BN_Pool
      */
      void add_affine(const PointGFp& other, BN_Pool& pool)
         {
         BOTAN_ASSERT_NOMSG(m_curve == other.m_curve);
         BOTAN_DEBUG_ASSERT(other.is_affine());

         const size_t p_words = m_curve.get_p_words();
         add_affine(other.m_coord_x.data(), std::min(p_words, other.m_coord_x.size()),
                    other.m_coord_y.data(), std::min(p_words, other.m_coord_y.size()),
                    pool);
         }

      /**
      * Point addition - mixed J+A. Array version.
      *
      * @param x_words the words of the x coordinate of the other point
      * @param x_size size of x_words
      * @param y_words the words of the y coordinate of the other point
      * @param y_size size of y_words
      * @param pool a BN_pool
      */
      void add_affine(const word x_words[], size_t x_size,
                      const word y_words[], size_t y_size,
                      BN_Pool& pool);

      /**
      * Point doubling
      * @param pool a BN_Pool
      */
      void mult2(BN_Pool& pool);

      /**
      * Repeated point doubling
      * @param i number of doublings to perform
      * @param pool a BN_pool
      */
      void mult2i(size_t i, BN_Pool& pool);

      /**
      * Point addition
      * @param other the point to add to *this
      * @param pool a BN_pool
      * @return other plus *this
      */
      PointGFp plus(const PointGFp& other, BN_Pool& pool) const
         {
         PointGFp x = (*this);
         x.add(other, pool);
         return x;
         }

      /**
      * Point doubling
      * @param pool a BN_Pool
      * @return *this doubled
      */
      PointGFp double_of(BN_Pool& pool) const
         {
         PointGFp x = (*this);
         x.mult2(pool);
         return x;
         }

      BigInt BOTAN_DEPRECATED("Use version taking a BN_Pool") get_affine_x() const
         {
         BN_Pool pool;
         return get_affine_x(pool);
         }

      BigInt BOTAN_DEPRECATED("Use version taking a BN_Pool") get_affine_y() const
         {
         BN_Pool pool;
         return get_affine_y(pool);
         }

      bool BOTAN_DEPRECATED("Use version taking a BN_Pool") on_the_curve() const
         {
         BN_Pool pool;
         return on_the_curve(pool);
         }

      static void BOTAN_DEPRECATED("Use version taking a BN_Pool")
         force_all_affine(std::vector<PointGFp>& points,
                          secure_vector<word>& ws)
         {
         BOTAN_UNUSED(ws);
         BN_Pool pool;
         force_all_affine(points, pool);
         }

      void swap_coords(BigInt& new_x, BigInt& new_y, BigInt& new_z)
         {
         m_coord_x.swap(new_x);
         m_coord_y.swap(new_y);
         m_coord_z.swap(new_z);
         }

      /**
      * Return base curve of this point
      * @result the curve over GF(p) of this point
      *
      * You should not need to use this
      */
      const CurveGFp& get_curve() const { return m_curve; }

   private:
      CurveGFp m_curve;
      BigInt m_coord_x, m_coord_y, m_coord_z;
   };

/**
* Point multiplication operator
* @param scalar the scalar value
* @param point the point value
* @return scalar*point on the curve
*/
BOTAN_PUBLIC_API(2,0) PointGFp operator*(const BigInt& scalar, const PointGFp& point);

/**
* ECC point multiexponentiation - not constant time!
* @param p1 a point
* @param z1 a scalar
* @param p2 a point
* @param z2 a scalar
* @result (p1 * z1 + p2 * z2)
*/
BOTAN_PUBLIC_API(2,0) PointGFp multi_exponentiate(
   const PointGFp& p1, const BigInt& z1,
   const PointGFp& p2, const BigInt& z2);

// relational operators
inline bool operator!=(const PointGFp& lhs, const PointGFp& rhs)
   {
   return !(rhs == lhs);
   }

// arithmetic operators
inline PointGFp operator-(const PointGFp& lhs)
   {
   return PointGFp(lhs).negate();
   }

inline PointGFp operator+(const PointGFp& lhs, const PointGFp& rhs)
   {
   PointGFp tmp(lhs);
   return tmp += rhs;
   }

inline PointGFp operator-(const PointGFp& lhs, const PointGFp& rhs)
   {
   PointGFp tmp(lhs);
   return tmp -= rhs;
   }

inline PointGFp operator*(const PointGFp& point, const BigInt& scalar)
   {
   return scalar * point;
   }

/**
* Perform point decoding
* Use EC_Group::OS2ECP instead
*/
PointGFp BOTAN_PUBLIC_API(2,0) OS2ECP(const uint8_t data[], size_t data_len,
                                      const CurveGFp& curve);

/**
* Perform point decoding
* Use EC_Group::OS2ECP instead
*
* @param data the encoded point
* @param data_len length of data in bytes
* @param curve_p the curve equation prime
* @param curve_a the curve equation a parameter
* @param curve_b the curve equation b parameter
*/
std::pair<BigInt, BigInt> BOTAN_UNSTABLE_API OS2ECP(const uint8_t data[], size_t data_len,
                                                    const BigInt& curve_p,
                                                    const BigInt& curve_a,
                                                    const BigInt& curve_b);

template<typename Alloc>
PointGFp OS2ECP(const std::vector<uint8_t, Alloc>& data, const CurveGFp& curve)
   { return OS2ECP(data.data(), data.size(), curve); }

}

namespace std {

template<>
inline void swap<Botan::PointGFp>(Botan::PointGFp& x, Botan::PointGFp& y)
   { x.swap(y); }

}

#endif
