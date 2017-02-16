/*
 *  Screen.cpp
 *  2Term
 *
 *  Created by Kelvin Sherlock on 7/7/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Screen.h"

#include <algorithm>



Screen::Screen(unsigned height, unsigned width)
{
    
    _frame = iRect(0, 0, width, height);

    _screen.resize(height);
    
    
    for (auto &line : _screen) line.resize(width);
}

Screen::~Screen()
{
}


void Screen::beginUpdate()
{
    _lock.lock();
    _updates.clear();
    _updateCursor = cursor();
}

iRect Screen::endUpdate()
{
    int maxX = -1;
    int maxY = -1;
    int minX = width();
    int minY = height();
    
    iPoint c = cursor();
    
    if (c != _updateCursor)
    {
        _updates.push_back(c);
        _updates.push_back(_updateCursor);
    }

    for (auto &point : _updates) {
        maxX = std::max(maxX, point.x);
        maxY = std::max(maxY, point.y);
        
        minX = std::min(minX, point.x);
        minY = std::min(minY, point.y);
    }
    
    _lock.unlock();
    
    return iRect(iPoint(minX, minY), iSize(maxX + 1 - minX, maxY + 1 - minY)); 
}




void Screen::putc(uint8_t c, const context &ctx)
{
    auto cursor = ctx.cursor;
    if (!_frame.contains(cursor)) return;

    _updates.push_back(cursor);
    _screen[cursor.y][cursor.x] = char_info(c, ctx.flags);
}





#pragma mark -
#pragma mark Cursor manipulation.




#pragma mark -
#pragma mark Erase

void Screen::eraseScreen()
{

    for (auto &line : _screen) {
        std::fill(line.begin(), line.end(), char_info());
    }
    _updates.push_back(iPoint(0,0));
    _updates.push_back(iPoint(width() - 1, height() - 1));
}
    

void Screen::eraseRect(iRect rect) {

    rect = rect.intersection(_frame);

    if (!rect.valid()) return;

    if (rect == _frame) return eraseScreen();

    auto yIter = _screen.begin() + rect.origin.y;
    auto yEnd = yIter + rect.size.height;
    
    while (yIter != yEnd) {
        auto &line = *yIter++;
        auto xIter = line.begin() + rect.origin.x;
        auto xEnd = xIter + rect.size.width;
        
        std::fill(xIter, xEnd, char_info());
    }
    _updates.push_back(rect.origin);
    _updates.push_back(iPoint(rect.maxX()-1, rect.maxY()-1));
}




template< class BidirIt1, class BidirIt2, class FX >
BidirIt2 copy_backward(BidirIt1 first, BidirIt1 last, BidirIt2 d_last, FX fx)
{
    while (first != last) {
        
        fx(*(--last), *(--d_last));
    }
    return d_last;
}

template<class InputIt, class OutputIt, class FX>
OutputIt copy_forward(InputIt first, InputIt last, OutputIt d_first, FX fx)
{
    while (first != last) {
        fx( *first, *d_first);
        ++first;
        ++d_first;
    }
    return d_first;
}

void Screen::scrollUp()
{
    // save the first line (to avoid allocation/deallocation)
    std::vector<char_info> tmp;
    std::swap(tmp, _screen.front());
    std::fill(tmp.begin(), tmp.end(), char_info());

    auto iter = std::move(_screen.begin() + 1, _screen.end(), _screen.begin());
    
    *iter = std::move(tmp);
    
    _updates.push_back(iPoint(0,0));
    _updates.push_back(iPoint(width() - 1, height() - 1));
}


void Screen::scrollUp(iRect rect)
{
    
    rect = rect.intersection(_frame);
    
    if (!rect.valid()) return;
    
    if (rect == _frame) return scrollUp();
    
    
    auto src = _screen.begin() + rect.minY()+1;
    auto dest = _screen.begin() + rect.minY();
    auto end = _screen.begin() + rect.maxY();
    
    auto iter = copy_forward(src, end, dest, [=](const auto &src, auto &dest){
        std::copy(src.begin() + rect.minX(), src.begin() + rect.maxX(), dest.begin());
    });

    std::fill(iter->begin() + rect.minX(), iter->begin() + rect.maxX(), char_info());
    
    
    _updates.push_back(rect.origin);
    _updates.push_back(iPoint(rect.maxX()-1, rect.maxY()-1));
}



void Screen::scrollDown()
{

    // save the first line (to avoid allocation/deallocation)
    std::vector<char_info> tmp;
    std::swap(tmp, _screen.back());
    std::fill(tmp.begin(), tmp.end(), char_info());
    
    auto iter = std::move_backward(_screen.begin(), _screen.end()-1, _screen.end());
    
    --iter;
    *iter = std::move(tmp);
    
    _updates.push_back(iPoint(0,0));
    _updates.push_back(iPoint(width() - 1, height() - 1));
    
}


void Screen::scrollDown(iRect rect)
{
    
    rect = rect.intersection(_frame);
    
    if (!rect.valid()) return;
    
    if (rect == _frame) return scrollDown();
    
    auto src = _screen.begin() + rect.minY();
    auto end = _screen.begin() + rect.maxY()-1;
    auto dest = _screen.begin() + rect.maxY();

    auto iter = copy_backward(src, end, dest, [=](const auto &src, auto &dest) {
        std::copy(src.begin() + rect.minX(), src.begin() + rect.maxX(), dest.begin());
    });
    --iter;
    std::fill(iter->begin() + rect.minX(), iter->begin() + rect.maxX(), char_info());


    _updates.push_back(rect.origin);
    _updates.push_back(iPoint(rect.maxX()-1, rect.maxY()-1));
}





#if 0
void Screen::scrollDown(iRect rect)
{
    
    rect = rect.intersection(_frame);

    if (!rect.valid()) return;
    
    if (rect == _frame) return scrollDown();
    
    
    auto src = _screen.begin() + rect.maxY()-1;
    auto dest = _screen.begin() + rect.maxY();
    auto end = _screen.begin() + rect.minY();
    
    
    while (src != end) {
        --src;
        --dest;

        std::copy(src->begin() + rect.minX(), src->begin() + rect.maxX(), dest->begin() + rect.minX());

    }
    
    auto &line = _screen[rect.minY()];
    std::fill(line.begin() + rect.minX(), line.begin() + rect.maxX(), char_info());
    
    _updates.push_back(rect.origin);
    _updates.push_back(iPoint(rect.maxX()-1, rect.maxY()-1));
}
#endif





void Screen::setSize(unsigned w, unsigned h)
{
    // TODO -- have separate minimum size for textport?

    if ((height() == h) && (width() == w)) return;
    
    _screen.resize(h);
    for (auto &line : _screen) {
        line.resize(w);
    }
    
    _frame.size = iSize(w, h);

    if (_cursor.y >= h) _cursor.y = h - 1;
    if (_cursor.x >= w) _cursor.x = w - 1;
    
    //fprintf(stderr, "setSize(%u, %u)\n", width, height);
        
}


void Screen::setCursorType(CursorType cursorType)
{
    _cursorType = cursorType;
}
