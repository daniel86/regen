/*
 * test-events.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <iostream>

#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <matrix.h>
#include <vector.h>
//#include <gl-frustum.h>

class TestAlgebra : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(TestAlgebra);
  CPPUNIT_TEST(testGetter);
  CPPUNIT_TEST(testSetter);
  CPPUNIT_TEST(testSpecialMatrix);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void)
  {
  }
  void tearDown(void)
  {
  }

protected:
  void testGetter(void)
  {
	  // TODO: write tests
  }
  void testSetter(void)
  {
	  // TODO: write tests
  }
  void testSpecialMatrix(void)
  {
	  // TODO: write tests
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestAlgebra);

