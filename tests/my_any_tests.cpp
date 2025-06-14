#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include "../src/IMetrics/MyAny.h"

using namespace MyCustomAny;

class MyAnyTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MyAnyTest, DefaultConstructor) {
    MyAny any;
    EXPECT_FALSE(any.has_value());
    EXPECT_EQ(any.type(), typeid(void));
}

TEST_F(MyAnyTest, ValueConstructorInt) {
    MyAny any(42);
    EXPECT_TRUE(any.has_value());
    EXPECT_EQ(any.type(), typeid(int));
}

TEST_F(MyAnyTest, ValueConstructorString) {
    MyAny any(std::string("hello"));
    EXPECT_TRUE(any.has_value());
    EXPECT_EQ(any.type(), typeid(std::string));
}

TEST_F(MyAnyTest, ValueConstructorDouble) {
    MyAny any(3.14);
    EXPECT_TRUE(any.has_value());
    EXPECT_EQ(any.type(), typeid(double));
}

TEST_F(MyAnyTest, CopyConstructorEmpty) {
    MyAny original;
    MyAny copy(original);
    EXPECT_FALSE(copy.has_value());
    EXPECT_EQ(copy.type(), typeid(void));
}

TEST_F(MyAnyTest, CopyConstructorWithValue) {
    MyAny original(42);
    MyAny copy(original);
    EXPECT_TRUE(copy.has_value());
    EXPECT_EQ(copy.type(), typeid(int));
    EXPECT_EQ(MyAnyCast<int>(copy), 42);
}

TEST_F(MyAnyTest, MoveConstructorEmpty) {
    MyAny original;
    MyAny moved(std::move(original));
    EXPECT_FALSE(moved.has_value());
    EXPECT_EQ(moved.type(), typeid(void));
}

TEST_F(MyAnyTest, MoveConstructorWithValue) {
    MyAny original(42);
    MyAny moved(std::move(original));
    EXPECT_TRUE(moved.has_value());
    EXPECT_EQ(moved.type(), typeid(int));
    EXPECT_EQ(MyAnyCast<int>(moved), 42);
}

TEST_F(MyAnyTest, CopyAssignmentEmpty) {
    MyAny original;
    MyAny target(42);
    target = original;
    EXPECT_FALSE(target.has_value());
    EXPECT_EQ(target.type(), typeid(void));
}

TEST_F(MyAnyTest, CopyAssignmentWithValue) {
    MyAny original(42);
    MyAny target;
    target = original;
    EXPECT_TRUE(target.has_value());
    EXPECT_EQ(target.type(), typeid(int));
    EXPECT_EQ(MyAnyCast<int>(target), 42);
}

TEST_F(MyAnyTest, CopyAssignmentSelf) {
    MyAny any(42);
    any = any;
    EXPECT_TRUE(any.has_value());
    EXPECT_EQ(any.type(), typeid(int));
    EXPECT_EQ(MyAnyCast<int>(any), 42);
}

TEST_F(MyAnyTest, MoveAssignmentEmpty) {
    MyAny original;
    MyAny target(42);
    target = std::move(original);
    EXPECT_FALSE(target.has_value());
    EXPECT_EQ(target.type(), typeid(void));
}

TEST_F(MyAnyTest, MoveAssignmentWithValue) {
    MyAny original(42);
    MyAny target;
    target = std::move(original);
    EXPECT_TRUE(target.has_value());
    EXPECT_EQ(target.type(), typeid(int));
    EXPECT_EQ(MyAnyCast<int>(target), 42);
}

TEST_F(MyAnyTest, Reset) {
    MyAny any(42);
    EXPECT_TRUE(any.has_value());
    any.reset();
    EXPECT_FALSE(any.has_value());
    EXPECT_EQ(any.type(), typeid(void));
}

TEST_F(MyAnyTest, HasValueEmpty) {
    MyAny any;
    EXPECT_FALSE(any.has_value());
}

TEST_F(MyAnyTest, HasValueWithValue) {
    MyAny any(42);
    EXPECT_TRUE(any.has_value());
}

TEST_F(MyAnyTest, TypeEmpty) {
    MyAny any;
    EXPECT_EQ(any.type(), typeid(void));
}

TEST_F(MyAnyTest, TypeInt) {
    MyAny any(42);
    EXPECT_EQ(any.type(), typeid(int));
}

TEST_F(MyAnyTest, TypeString) {
    MyAny any(std::string("test"));
    EXPECT_EQ(any.type(), typeid(std::string));
}

TEST_F(MyAnyTest, TypeVector) {
    MyAny any(std::vector<int>{1, 2, 3});
    EXPECT_EQ(any.type(), typeid(std::vector<int>));
}

TEST_F(MyAnyTest, EqualityBothEmpty) {
    MyAny any1;
    MyAny any2;
    EXPECT_TRUE(any1 == any2);
}

TEST_F(MyAnyTest, EqualityOneEmpty) {
    MyAny any1;
    MyAny any2(42);
    EXPECT_FALSE(any1 == any2);
    EXPECT_FALSE(any2 == any1);
}

TEST_F(MyAnyTest, EqualitySameTypeAndValue) {
    MyAny any1(42);
    MyAny any2(42);
    EXPECT_TRUE(any1 == any2);
}

TEST_F(MyAnyTest, EqualitySameTypeDifferentValue) {
    MyAny any1(42);
    MyAny any2(24);
    EXPECT_FALSE(any1 == any2);
}

TEST_F(MyAnyTest, EqualityDifferentTypes) {
    MyAny any1(42);
    MyAny any2(42.0);
    EXPECT_FALSE(any1 == any2);
}

TEST_F(MyAnyTest, EqualityStrings) {
    MyAny any1(std::string("hello"));
    MyAny any2(std::string("hello"));
    MyAny any3(std::string("world"));
    EXPECT_TRUE(any1 == any2);
    EXPECT_FALSE(any1 == any3);
}

TEST_F(MyAnyTest, EqualityVectors) {
    MyAny any1(std::vector<int>{1, 2, 3});
    MyAny any2(std::vector<int>{1, 2, 3});
    MyAny any3(std::vector<int>{1, 2, 4});
    EXPECT_TRUE(any1 == any2);
    EXPECT_FALSE(any1 == any3);
}

TEST_F(MyAnyTest, MyAnyCastLvalueReference) {
    MyAny any(42);
    const int& value = MyAnyCast<int>(any);
    EXPECT_EQ(value, 42);
}

TEST_F(MyAnyTest, MyAnyCastConstReference) {
    const MyAny any(42);
    const int& value = MyAnyCast<int>(any);
    EXPECT_EQ(value, 42);
}

TEST_F(MyAnyTest, MyAnyCastRvalueReference) {
    MyAny any(42);
    int value = MyAnyCast<int>(std::move(any));
    EXPECT_EQ(value, 42);
}

TEST_F(MyAnyTest, MyAnyCastString) {
    MyAny any(std::string("hello"));
    std::string value = MyAnyCast<std::string>(any);
    EXPECT_EQ(value, "hello");
}

TEST_F(MyAnyTest, MyAnyCastVector) {
    std::vector<int> original{1, 2, 3};
    MyAny any(original);
    std::vector<int> value = MyAnyCast<std::vector<int>>(any);
    EXPECT_EQ(value, original);
}

TEST_F(MyAnyTest, MyAnyCastEmptyThrows) {
    MyAny any;
    EXPECT_THROW(MyAnyCast<int>(any), std::bad_any_cast);
}

TEST_F(MyAnyTest, MyAnyCastWrongTypeThrows) {
    MyAny any(42);
    EXPECT_THROW(MyAnyCast<std::string>(any), std::bad_any_cast);
}

TEST_F(MyAnyTest, MyAnyCastConstEmptyThrows) {
    const MyAny any;
    EXPECT_THROW(MyAnyCast<int>(any), std::bad_any_cast);
}

TEST_F(MyAnyTest, MyAnyCastConstWrongTypeThrows) {
    const MyAny any(42);
    EXPECT_THROW(MyAnyCast<std::string>(any), std::bad_any_cast);
}

TEST_F(MyAnyTest, MyAnyCastRvalueEmptyThrows) {
    MyAny any;
    EXPECT_THROW(MyAnyCast<int>(std::move(any)), std::bad_any_cast);
}

TEST_F(MyAnyTest, MyAnyCastRvalueWrongTypeThrows) {
    MyAny any(42);
    EXPECT_THROW(MyAnyCast<std::string>(std::move(any)), std::bad_any_cast);
}

TEST_F(MyAnyTest, CopyConstructorDeepCopy) {
    MyAny original(42);
    MyAny copy(original);
    MyAnyCast<int>(original) = 24;
    EXPECT_EQ(MyAnyCast<int>(copy), 42);
    EXPECT_EQ(MyAnyCast<int>(original), 24);
}

TEST_F(MyAnyTest, CopyAssignmentDeepCopy) {
    MyAny original(42);
    MyAny copy;
    copy = original;
    MyAnyCast<int>(original) = 24;
    EXPECT_EQ(MyAnyCast<int>(copy), 42);
    EXPECT_EQ(MyAnyCast<int>(original), 24);
}

TEST_F(MyAnyTest, ComplexObjectStorage) {
    struct TestStruct {
        int x;
        std::string y;
        bool operator==(const TestStruct& other) const {
            return x == other.x && y == other.y;
        }
    };
    
    TestStruct original{42, "test"};
    MyAny any(original);
    TestStruct retrieved = MyAnyCast<TestStruct>(any);
    EXPECT_EQ(retrieved.x, 42);
    EXPECT_EQ(retrieved.y, "test");
}

TEST_F(MyAnyTest, ComplexObjectEquality) {
    struct TestStruct {
        int x;
        std::string y;
        bool operator==(const TestStruct& other) const {
            return x == other.x && y == other.y;
        }
    };
    
    MyAny any1(TestStruct{42, "test"});
    MyAny any2(TestStruct{42, "test"});
    MyAny any3(TestStruct{24, "test"});
    
    EXPECT_TRUE(any1 == any2);
    EXPECT_FALSE(any1 == any3);
}

TEST_F(MyAnyTest, TypeAfterReset) {
    MyAny any(42);
    EXPECT_EQ(any.type(), typeid(int));
    any.reset();
    EXPECT_EQ(any.type(), typeid(void));
}

TEST_F(MyAnyTest, ReassignmentDifferentTypes) {
    MyAny any(42);
    EXPECT_EQ(any.type(), typeid(int));
    any = MyAny(std::string("hello"));
    EXPECT_EQ(any.type(), typeid(std::string));
    EXPECT_EQ(MyAnyCast<std::string>(any), "hello");
}

TEST_F(MyAnyTest, MultipleReassignments) {
    MyAny any;
    
    any = MyAny(42);
    EXPECT_EQ(MyAnyCast<int>(any), 42);
    
    any = MyAny(3.14);
    EXPECT_EQ(MyAnyCast<double>(any), 3.14);
    
    any = MyAny(std::string("test"));
    EXPECT_EQ(MyAnyCast<std::string>(any), "test");
    
    any.reset();
    EXPECT_FALSE(any.has_value());
}

TEST_F(MyAnyTest, MoveSemantics) {
    std::string original = "test string";
    MyAny any(std::move(original));
    EXPECT_EQ(MyAnyCast<std::string>(any), "test string");
}

TEST_F(MyAnyTest, EqualityAfterModification) {
    MyAny any1(42);
    MyAny any2(42);
    EXPECT_TRUE(any1 == any2);
    
    MyAnyCast<int>(any1) = 24;
    EXPECT_FALSE(any1 == any2);
    EXPECT_EQ(MyAnyCast<int>(any1), 24);
    EXPECT_EQ(MyAnyCast<int>(any2), 42);
}

TEST_F(MyAnyTest, LargeObjectStorage) {
    std::vector<int> large_vector(1000, 42);
    MyAny any(large_vector);
    std::vector<int> retrieved = MyAnyCast<std::vector<int>>(any);
    EXPECT_EQ(retrieved.size(), 1000);
    EXPECT_EQ(retrieved[0], 42);
    EXPECT_EQ(retrieved[999], 42);
}

TEST_F(MyAnyTest, PointerStorage) {
    int value = 42;
    int* ptr = &value;
    MyAny any(ptr);
    int* retrieved = MyAnyCast<int*>(any);
    EXPECT_EQ(retrieved, ptr);
    EXPECT_EQ(*retrieved, 42);
}

TEST_F(MyAnyTest, SharedPointerStorage) {
    auto ptr = std::make_shared<int>(42);
    MyAny any(ptr);
    auto retrieved = MyAnyCast<std::shared_ptr<int>>(any);
    EXPECT_EQ(retrieved, ptr);
    EXPECT_EQ(*retrieved, 42);
}