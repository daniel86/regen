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

#include <event-object.h>

void handler0(EventObject*, void*);
void handler1(EventObject*, void*);

class TestEvents : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(TestEvents);
  CPPUNIT_TEST(testEvents);
  CPPUNIT_TEST_SUITE_END();

public:
  EventObject object;
  enum{
	  EVENT0,EVENT1,EVENT2,EVENT_LAST
  };
  int eventIds[EVENT_LAST];
  int debugValue;

  void setUp(void)
  {
	  eventIds[EVENT0] = object.registerEvent("0");
	  eventIds[EVENT1] = object.registerEvent("1");
	  eventIds[EVENT2] = object.registerEvent("2");
	  debugValue = 0;
  }
  void tearDown(void)
  {
  }

protected:

  void testEvents(void)
  {
	  int handlerId0 = object.connect("0", handler0, this);
	  int handlerId1 = object.connect(eventIds[EVENT1], handler1, this);

	  CPPUNIT_ASSERT( debugValue == 0 );
	  object.emit("0");
	  CPPUNIT_ASSERT( debugValue == 1 );
	  object.emit(eventIds[EVENT1]);
	  CPPUNIT_ASSERT( debugValue == 0 );

	  object.disconnect("1", handlerId1);
	  handlerId1 = object.connect("0", handler1, this);

	  object.emit("0");
	  CPPUNIT_ASSERT( debugValue == 0 );

	  object.disconnect(eventIds[EVENT0], handlerId0);
	  object.disconnect("0", handlerId1);
  }
};

void handler0(EventObject*, void *data)
{
	TestEvents *test = static_cast<TestEvents*>(data);
	test->debugValue += 1;
}
void handler1(EventObject*, void *data)
{
	TestEvents *test = static_cast<TestEvents*>(data);
	test->debugValue -= 1;
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestEvents);

