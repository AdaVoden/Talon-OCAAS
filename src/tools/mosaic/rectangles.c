
#include "rectangles.h"

/******************************************************************************\
	Rectangle functions
\******************************************************************************/

void SetRect(RectPtr pRect, int l, int t, int r, int b)
{
	if(pRect) {
		pRect->left = l;
		pRect->top = t;
		pRect->right = r;
		pRect->bottom = b;
	}
}
void SetRectWH(RectPtr pRect, int l, int t, int w, int h)
{
	SetRect(pRect,l,t,l+w,t+h);
}
BOOL IsPointInRect(RectPtr pRect, int x, int y)
{
	if(pRect) {
		return(x >= pRect->left && x < pRect->right
		&& y >= pRect->top && y < pRect->bottom);
	}
	return FALSE;	
}
BOOL IntersectRect(RectPtr pRectOut, RectPtr pRect1, RectPtr pRect2)
{
	register RectPtr r1 = pRect1;
	register RectPtr r2 = pRect2;
	register RectPtr d = pRectOut;
 	return (
         (
          (d->left = ((r1->left > r2->left) ? r1->left : r2->left)) <
          (d->right = ((r1->right < r2->right) ? r1->right : r2->right))
         )
        &
         (
          (d->top = ((r1->top > r2->top) ? r1->top : r2->top)) <
          (d->bottom = ((r1->bottom < r2->bottom) ? r1->bottom : r2->bottom))
         )
    );
}
BOOL MergeRect(RectPtr pRectOut, RectPtr pRect1, RectPtr pRect2)
{
	register RectPtr r1 = pRect1;
	register RectPtr r2 = pRect2;
	register RectPtr d = pRectOut;
    return (
         (
          (d->left = ((r1->left < r2->left) ? r1->left : r2->left)) <
          (d->right = ((r1->right > r2->right) ? r1->right : r2->right))
         )
        &
         (
          (d->top = ((r1->top < r2->top) ? r1->top : r2->top)) <
          (d->bottom = ((r1->bottom > r2->bottom) ? r1->bottom : r2->bottom))
         )
    );
}
void OffsetRect(RectPtr pRect, int dx, int dy)
{
	if(pRect) {
		pRect->left += dx;
		pRect->top += dy;
		pRect->right += dx;
		pRect->bottom += dy;
	}
}
int RectWidth(RectPtr r)
{
	if(r) {
		return(r->right - r->left);
	}
	return(0);
}
int RectHeight(RectPtr r)
{
	if(r) {
		return(r->bottom - r->top);
	}
	return(0);
}
