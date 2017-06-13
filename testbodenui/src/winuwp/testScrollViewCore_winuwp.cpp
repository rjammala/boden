#include <bdn/init.h>
#include <bdn/test.h>

#include <bdn/Window.h>
#include <bdn/test/TestScrollViewCore.h>
#include <bdn/winuwp/UiProvider.h>
#include "TestWinuwpViewCoreMixin.h"


using namespace bdn;

class TestWinuwpScrollViewCore : public bdn::test::TestWinuwpViewCoreMixin< bdn::test::TestScrollViewCore >
{
protected:

    
  
    double getVertBarWidth()
    {
        return -1;
    }


    double getHorzBarHeight()
    {
        return -1;
    }
                

    void resizeScrollViewToViewPortSize( const Size& viewPortSize)
    {
    }
    
    void verifyHorzBarVisible( bool expectedVisible) override
    {
    }

    void verifyVertBarVisible( bool expectedVisible) override
    {
    }

    void verifyContentViewBounds( const Rect& expectedBounds, double maxDeviation=0) override
    {
    }

    void verifyScrolledAreaSize( const Size& expectedSize) override
    {
    }

    void verifyViewPortSize( const Size& expectedSize) override
    {
    }               
             
    void verifyCorePadding() override
    {
    }


};

TEST_CASE("winuwp.ScrollViewCore")
{
    P<TestWinuwpScrollViewCore> pTest = newObj<TestWinuwpScrollViewCore>();

    pTest->runTests();
}

