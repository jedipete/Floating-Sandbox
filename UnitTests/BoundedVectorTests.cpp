#include <GameCore/BoundedVector.h>

#include <vector>

#include "gtest/gtest.h"

TEST(BoundedVectorTests, DefaultCctorMakesZeroSize)
{
    BoundedVector<int> vec;

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, CctorWithMaxSize)
{
    BoundedVector<int> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ClearOnZeroSize)
{
    BoundedVector<int> vec;

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.clear();

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ClearOnEmpty)
{
    BoundedVector<int> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.clear();

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, EmplaceBack)
{
    struct Elem
    {
        int val1;
        float val2;

        Elem(
            int _val1,
            float _val2)
            : val1(_val1)
            , val2(_val2)
        {}
    };

    BoundedVector<Elem> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    Elem & foo1 = vec.emplace_back(4, 0.4f);

    EXPECT_EQ(1u, vec.size());
    EXPECT_EQ(4, vec.back().val1);
    EXPECT_EQ(0.4f, vec.back().val2);
    EXPECT_EQ(4, foo1.val1);
    EXPECT_EQ(0.4f, foo1.val2);
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    Elem & foo2 = vec.emplace_back(2, 0.2f);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2, vec.back().val1);
    EXPECT_EQ(0.2f, vec.back().val2);
    EXPECT_EQ(2, foo2.val1);
    EXPECT_EQ(0.2f, foo2.val2);
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());
}

TEST(BoundedVectorTests, Reset)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.reset(3);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ResetToZero)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.reset(0);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ResetOnZeroSize)
{
    BoundedVector<int> vec;

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.reset(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ResetOnEmpty)
{
    BoundedVector<int> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.reset(3);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, Back)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(5);
    vec.emplace_back(6);

    ASSERT_EQ(2, vec.size());

    EXPECT_EQ(6, vec.back());
}

TEST(BoundedVectorTests, Indexer)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(5);
    vec.emplace_back(6);

    ASSERT_EQ(2, vec.size());

    EXPECT_EQ(5, vec[0]);
    EXPECT_EQ(6, vec[1]);
}