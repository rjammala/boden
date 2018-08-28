#ifndef BDN_GTK_ScrollViewCore_H_
#define BDN_GTK_ScrollViewCore_H_


#include <bdn/IScrollViewCore.h>
#include <bdn/ScrollView.h>
#include <bdn/ScrollViewLayoutHelper.h>
#include <bdn/gtk/ViewCore.h>



namespace bdn
{
namespace gtk
{

class ScrollViewCore : public ViewCore, BDN_IMPLEMENTS IScrollViewCore
{
private:
    static GtkWidget* _createScrolledWindow( ScrollView* pOuter )
    {
        GtkWidget* pScrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
        
        return pScrolledWindow;
    }
    
public:
    ScrollViewCore(View* pOuter)
    : ViewCore( pOuter,
                _createScrolledWindow( cast<ScrollView>(pOuter) ) )
    {   
        // note: we do not use our GTK Bridge widget here since
        // GtkScrolledWindow does not fully pass size requests on to it anyway
        // (Width-for-height and height-for-width tradeoffs are not used).
        // Because of that we have to explicitly set the desired content size
        // anyway, so the bridge would not offer any benefits. In fact, it would
        // interfere because its unconditional preferred size IS taken into account by
        // the scroll window. In contrast, a simple GtkLayout allows us to have full control
        // at a single point.
        _pScrolledArea = gtk_layout_new(nullptr, nullptr);
        gtk_container_add( GTK_CONTAINER(getGtkWidget()), _pScrolledArea);
                
        gtk_widget_set_visible( _pScrolledArea, TRUE );
        
        updateScrollPolicy();
        
        GtkScrolledWindow* pScrolledWindow = GTK_SCROLLED_WINDOW( getGtkWidget() );
        
        _pHAdjustment = gtk_scrolled_window_get_hadjustment( pScrolledWindow );
        _pVAdjustment = gtk_scrolled_window_get_vadjustment( pScrolledWindow );
        
        g_signal_connect( _pHAdjustment, "value-changed", G_CALLBACK(scrollPositionChangedCallback), this );
        g_signal_connect( _pVAdjustment, "value-changed", G_CALLBACK(scrollPositionChangedCallback), this );        
    }
    
    
    void setPadding(const Nullable<UiMargin>& uiPadding) override
    {
        // nothing to do. We apply the padding in calcPreferredSize and layout.
        /*Margin padding;
        
        if(!uiPadding.isNull())
            padding = uiMarginToDipMargin(uiPadding);
        
        bdn_gtk_bridge_set_padding( BDN_GTK_BRIDGE( _pScrolledAreaBridge ), padding );        */
    }
    
        
    void setHorizontalScrollingEnabled(const bool& enabled) override
    {
        updateScrollPolicy();
    }

    void setVerticalScrollingEnabled(const bool& enabled) override
    {
        updateScrollPolicy();
    }
    
    
    void scrollClientRectToVisible(const Rect& targetRect) override
    {
        double left = targetRect.x;
        double right = targetRect.x + targetRect.width;
        double top = targetRect.y;
        double bottom = targetRect.y + targetRect.height;
        
        // first, clip the target rect to the client area.
        // This also automatically gets rid of infinity target positions (which are allowed)
        if( left > _clientSize.width)
            left = _clientSize.width;
        if( right > _clientSize.width )
            right = _clientSize.width;
        if( top > _clientSize.height)
            top = _clientSize.height;
        if( bottom > _clientSize.height )
            bottom = _clientSize.height;
        
        if(left<0)
            left = 0;
        if(right<0)
            right = 0;
        if(top<0)
            top = 0;
        if(bottom<0)
            bottom = 0;
        
        if(right<left)
            right = left;
        if(bottom<top)
            bottom = top;
        
        Point scrollPos = getGtkScrollPosition();
        
        // there is a special case if the target rect is bigger than the viewport.
        // In that case the desired end position is ambiguous: any sub-rect of viewport size
        // inside the specified target rect would be "as good as possible".
        // The documentation for scrollClientRectToVisible resolves this ambiguity by requiring
        // that we scroll the minimal amount. So we want the new visible rect to be as close
        // to the old one as possible.
        
        double currVisibleLeft = scrollPos.x;
        double currVisibleRight = currVisibleLeft + _viewPortSize.width;
        double currVisibleTop = scrollPos.y;
        double currVisibleBottom = currVisibleTop + _viewPortSize.height;
        
        if(right-left > _viewPortSize.width)
        {
            // the width of the target rect is bigger than the viewport width.
            // So not all of the target rect can be made visible.
            // We want the closest edge to the current visible rect to be prioritized.
            
            if( currVisibleLeft >= left && currVisibleRight<=right)
            {
                // The current visible rect is already fully inside the target rect.
                // In this case we do not want to move the scroll position at all.
                // So set the target rect to the current view port rect
                left = currVisibleLeft;
                right = currVisibleRight;
            }
            else
            {
                // shrink the target rect so that it matches the viewport width.
                // We want to shrink towards the edge that is closest to the current visible rect.
                // Note that the width of the visible rect is smaller than the target width and
                // that the visible rect is not fully inside the target rect.
                // So one of the target rect edges has to be closer than the other.
                
                double distanceLeft = fabs( left-currVisibleLeft );
                double distanceRight = fabs( right-currVisibleRight );
                
                if(distanceLeft<distanceRight)
                {
                    // the left edge of the target rect is closer to the current visible rect
                    // than the right edge. So we want to move towards the left.
                    right = left + _viewPortSize.width;
                }
                else
                {
                    // move towards the right edge
                    left = right - _viewPortSize.width;
                }
            }
        }
        
        if(bottom-top > _viewPortSize.height)
        {
            if( currVisibleTop >= top && currVisibleBottom<=bottom)
            {
                top = currVisibleTop;
                bottom = currVisibleBottom;
            }
            else
            {
                double distanceTop = fabs( top-currVisibleTop );
                double distanceBottom = fabs( bottom-currVisibleBottom );
                
                if(distanceTop<distanceBottom)
                    bottom = top + _viewPortSize.height;
                else
                    top = bottom - _viewPortSize.height;
            }
        }
        
        if(left<0)
            left = 0;
        if(right<0)
            right = 0;
        if(top<0)
            top = 0;
        if(bottom<0)
            bottom = 0;            

		int scrollX = currVisibleLeft;
		int scrollY = currVisibleTop;

		if(right > currVisibleRight)
			scrollX = right - _viewPortSize.width;
		if(left < currVisibleLeft)
			scrollX = left;

		if(bottom > currVisibleBottom)
			scrollY = bottom - _viewPortSize.height;
		if(top < currVisibleTop)
			scrollY = top;

        if(_pHAdjustment!=nullptr)
            gtk_adjustment_set_value(_pHAdjustment, scrollX);
        if(_pVAdjustment!=nullptr)
            gtk_adjustment_set_value(_pVAdjustment, scrollY);        
    }
    
	Size calcPreferredSize( const Size& availableSpace = Size::none() ) const override
	{
        // When it calculates its preferred size, GtkScrolledWindow does not actually take the child size into account, unless
        // gtk_scrolled_window_get_propagate_natural_width etc. are set to TRUE.
        // But gtk_scrolled_window_get_propagate_natural_XYZ are relatively new (GTK 3.22 or newer)
        // So we do not want to depend on those.
                
        // There is also a second problem. GtkScrolledWindow does not implement the "height for width" or
        // "width for height" sizing at all. Even with the "propagate natural size", it does not pass the
        // available width/height on to the child, so the child/ has no way to adapt to it. It only asks 
        
        // All in all that means that we have to calculate the preferred size of the scrolled area and the
        // scroll view.
        
        // BUT if we want to do that then we need to know the size of the nonclient stuff that scrolled window
        // shows. Border sizes, sizes of scrollbars, etc.
        // Whether or not a border is shown is controlled by the scrolled child window in GTK. Since we provide that
        // (our bridge) that means that no border is shown.
        
        Size preferredSize;
        
        P<ScrollView> pOuter = cast<ScrollView>( getOuterViewIfStillAttached() );
        if(pOuter!=nullptr)
        {        
            P<ScrollViewLayoutHelper> pHelper = createLayoutHelper();
                
            preferredSize = pHelper->calcPreferredSize( pOuter, availableSpace );
        }
        
        return preferredSize;
	}

        
	void layout() override
	{
        // we have to calculate the size we want our content view to have and then
        // set that as the min content size in the gtk scrolled window.
        
        P<ScrollView> pOuter = cast<ScrollView>( getOuterViewIfStillAttached() );
        if(pOuter!=nullptr)
        {        
            // we do not have any borders, so the viewport size without scrollbars
            // is the full scrollview size.
            Size viewPortSize = pOuter->size();
            
            P<ScrollViewLayoutHelper> pHelper = createLayoutHelper();        
            pHelper->calcLayout( pOuter, viewPortSize );
            
            _viewPortSize = pHelper->getViewPortSize();
            
            _clientSize = pHelper->getScrolledAreaSize();
            
            gtk_layout_set_size( GTK_LAYOUT(_pScrolledArea), _clientSize.width, _clientSize.height);
            
            //GtkScrolledWindow* pScrolledWindow = GTK_SCROLLED_WINDOW( getGtkWidget() );
            
            //gtk_scrolled_window_set_min_content_width( pScrolledWindow, scrolledAreaSize.width );
            //gtk_scrolled_window_set_min_content_height( pScrolledWindow, scrolledAreaSize.height );
            
            P<View> pContentView = pOuter->getContentView();
            if(pContentView!=nullptr)
                pContentView->adjustAndSetBounds( pHelper->getContentViewBounds() );
                
            updateVisibleClientRect();
        }
	}
    
    
    void _addChildViewCore(ViewCore* pChildCore) override
    {
        gtk_container_add( GTK_CONTAINER(_pScrolledArea), pChildCore->getGtkWidget() );
        
        //P<View> pChildView = pChildCore->getOuterViewIfStillAttached();

        //bdn_gtk_bridge_set_child( BDN_GTK_BRIDGE(_pScrolledAreaBridge), pChildView, pChildCore->getGtkWidget() );        
    }
    
    void _moveChildViewCore(ViewCore* pChildCore, int gtkX, int gtkY) override
    {
        gtk_layout_move( GTK_LAYOUT(_pScrolledArea), pChildCore->getGtkWidget(), gtkX, gtkY);
                
        /*
        // the content view CANNOT be moved. This is under the control of the scroll view
        // and the bridge widget.        */
    }    
        
        
        
    /** Returns the scrollbar spacing of the specified scrolled window.
     * 
     *  The spacing is the empty space between the scrollbars and the content.
     * */
    static int getScrollBarSpacing(GtkScrolledWindow* pScrolledWindow)
    {
        GtkScrolledWindowClass* pScrollClass = GTK_SCROLLED_WINDOW_GET_CLASS( GTK_SCROLLED_WINDOW(pScrolledWindow) );
        
        int scrollBarSpacing = 0;
        
        // looking at the gtk source code, the class can override the CSS value.
        // The default for the class value is -1, so this seems to be mainly there to allow
        // derived classes to change the spacing.

        if (pScrollClass->scrollbar_spacing >= 0)
            scrollBarSpacing = pScrollClass->scrollbar_spacing;
        else
        {
            gint spacing = 0;

            gtk_widget_style_get ( GTK_WIDGET(pScrolledWindow), "scrollbar-spacing", &spacing, NULL);
            
            scrollBarSpacing = spacing;
        }        
        
        return scrollBarSpacing;
    }
        
     
    /** Returns the amount of layout space that is allocated for the scrollbars when they
     *  are there.
     * 
     *  If overlay scrolling is used then no space is reserved for scrollbars, so the
     *  space values will be set to 0.
     * 
     *  When the returned space is >0 then it includes any applicable scrollbar spacing
     *  (see getScrollBarSpacing() ).
     * */
    static void getScrollBarLayoutSpaces(GtkScrolledWindow* pScrolledWindow, int& vertBarSpaceWidth, int& horzBarSpaceHeight)
    {   
       
        // The sizes of the scrollbars are a little complicated. GtkScrolledWindow supports "overlay" scrollbars
        // (which do not take up any layout space) as well as old-school fixed scrollbars.
        // According to the GTK documentation, which model is used depends on settings and on whether or not a mouse is connected.
        // At the time of this writing there is no "mouse connected" check in the GTK source code, but we must assume that they
        // might implement it in the future.
        // Since the triggers for overlay scrolling are unclear and subject to change, we cannot check for those.
        // We must look at the results and detect from that whether or not the overlay scrolling is used or not.
        
        // Looking at the GtkScrolledWindow source code (at the time of this writing) we can check the scrollbar parent
        // to see if overlay scrolling is used. If the parent is the scrolled window then scrollbars take up real
        // layout space. If it is some other window then overlay scrolling is used.
        // However, it seems risky to depend on this fact.
        
        // So, how else can we detect if scrollbars take up space?
        
        // we could set up dummy scrolled window with zero size content and then look at its minimum size.
        // If scrollbars are overlays then the min size is 0x0 (at the time of this writing).
        // If scrollbars are not overlays then the min size is >0.
        
        // We need a dummy scrolled window to do this, because any current layout information that is cached
        // for the real scroll window would interfere with this detection. And caches cannot be cleared
        // immediately - cache clearing is always posted to the event queue. So we need an independent scrolled window for this.
        
        // We would also need to keep the dummy window alive, since the behaviour can potentially change at runtime
        // when a mouse is connected - so we need to re-check in every layout cycle.
        
        // So, we have two ways to do this: one depends on some internal implementation knowledge and the
        // other needs a dummy scrolled window to be around at all time. Both are pretty hackish.
        
        // The scrollbar check seems to be the more reasonable one. It seems unlikely, that an overlay scrollbar would
        // be a "real" child of the scrolled window. So we use that approach.
        
        // UPDATE: it seemed unlikely, but in the current GTK master branch the parent of the overlay scrollbars
        // always remains the scrolled window. So we cannot use that to check for the indicators.
        
        // Luckily there is one other thing that seems to be a constant among all versions of GTK that supported
        // overlay scrollbars: when the scrollbar is in overlay mode then the "overlay-indicator" style class is
        // set for the scrollbar. This was the case for GTK 3.16 (the first one that supported overlay scrollbars)
        // and it is still the case for the current GTK 3 version as of the time of this writing (3.22)
        
        
        
        // if scrollbars are overlays then the scrolled window will have a minimum size of 0.
        // If scrollbars are not overlays then the scrolled window has a somewhat complicated minimum size,
        // a combination of multiple values.
        
        // in addition to the scrollbar sizes, there can also be spacing configured between the scrollbar
        // and the content. We must include that in our calculation as well.      

        vertBarSpaceWidth = 0;
        horzBarSpaceHeight = 0;
        
        GtkWidget* pHBar = gtk_scrolled_window_get_hscrollbar( pScrolledWindow );
        GtkWidget* pVBar = gtk_scrolled_window_get_vscrollbar( pScrolledWindow );
        
        bool barsTakeUpSpace = false;
        
        if(pHBar!=nullptr || pVBar!=nullptr)
        {        
            GtkWidget* pBar = (pVBar!=nullptr) ? pVBar : pHBar;
            
            GtkStyleContext* pStyleContext = gtk_widget_get_style_context (pBar);
            
            barsTakeUpSpace = (gtk_style_context_has_class( pStyleContext, "overlay-indicator" ) == 0);            
        }
        
        if(barsTakeUpSpace)
        {
            int scrollBarSpacing = getScrollBarSpacing(pScrolledWindow);
            
            if(pHBar!=nullptr)
            {                       
                gint barMinHeight=0;
                gint barNaturalHeight=0;
                gtk_widget_get_preferred_height ( pHBar, &barMinHeight, &barNaturalHeight );
                
                horzBarSpaceHeight = barNaturalHeight;      
            }
            
            if(pVBar!=nullptr)
            {                
                gint barMinWidth=0;
                gint barNaturalWidth=0;
                gtk_widget_get_preferred_width ( pVBar, &barMinWidth, &barNaturalWidth );
            
                vertBarSpaceWidth = barNaturalWidth;                
            }
            
            if(horzBarSpaceHeight>0)
                horzBarSpaceHeight += scrollBarSpacing;
            if(vertBarSpaceWidth>0)
                vertBarSpaceWidth += scrollBarSpacing;
        }        
    }
                
        
        
protected:
    bool canAdjustWidthToAvailableSpace() const override
    {
        return true;
    }


    bool canAdjustHeightToAvailableSpace() const override
    {
        return true;
    }
    
private:

    Point getGtkScrollPosition()
    {
        Point scrollPos;
            
        if(_pHAdjustment!=nullptr)
            scrollPos.x = gtk_adjustment_get_value( _pHAdjustment );
            
        if(_pVAdjustment!=nullptr)
            scrollPos.y = gtk_adjustment_get_value( _pVAdjustment );
            
        return scrollPos;
    }
    
    void updateVisibleClientRect()    
    {
        P<ScrollView> pOuter = cast<ScrollView>( getOuterViewIfStillAttached() );
        if(pOuter!=nullptr)
        {   
            Point scrollPos = getGtkScrollPosition();            

            Rect visibleRect( scrollPos, _viewPortSize );
        
            pOuter->_setVisibleClientRect(visibleRect);
        }                
    }
    
    
    void updateScrollPolicy()
    {
        P<ScrollView> pOuter = cast<ScrollView>( getOuterViewIfStillAttached() );
        if(pOuter!=nullptr)
        {        
            gtk_scrolled_window_set_policy(
                GTK_SCROLLED_WINDOW( getGtkWidget() ),
                pOuter->horizontalScrollingEnabled() ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER,
                pOuter->verticalScrollingEnabled() ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER );
        }
    }
    
    P<ScrollViewLayoutHelper> createLayoutHelper() const
    {
        int vertBarSpaceWidth;
        int horzBarSpaceHeight;        
        getScrollBarLayoutSpaces( GTK_SCROLLED_WINDOW(getGtkWidget()), vertBarSpaceWidth, horzBarSpaceHeight );
        
                        
        // now we can let our scrollview layout helper determine the preferred size.
        return newObj<ScrollViewLayoutHelper>( vertBarSpaceWidth, horzBarSpaceHeight);
    }
    
    void scrollPositionChanged()
    {
        updateVisibleClientRect();
    }
    
    static void scrollPositionChangedCallback(GtkAdjustment* pAdjustment, gpointer pParam)
    {
        ((ScrollViewCore*)pParam)->scrollPositionChanged();
    }   
       
    GtkWidget*      _pScrolledArea = nullptr;    
    GtkAdjustment*  _pHAdjustment = nullptr;
    GtkAdjustment*  _pVAdjustment = nullptr;
    
    Size            _viewPortSize;
    Size            _clientSize;
};


}
}

#endif