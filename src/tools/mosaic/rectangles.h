
// -------------------------------------------------
// Rectangle stuff.

// Define our rectangle structure
typedef struct
{
	int	left;
	int top;
	int right;
	int bottom;
} Rect, *RectPtr;

// define boolean type
typedef int BOOL;
#define FALSE (0)
#define TRUE  (!FALSE)

// our rectangle prototypes
void SetRect(RectPtr pRect, int l, int t, int r, int b);
void SetRectWH(RectPtr pRect, int l, int t, int w, int h);
BOOL IsPointInRect(RectPtr pRect, int x, int y);
BOOL IntersectRect(RectPtr pRectOut, RectPtr pRect1, RectPtr pRect2);
BOOL MergeRect(RectPtr pRectOut, RectPtr pRect1, RectPtr pRect2);
void OffsetRect(RectPtr pRect, int dx, int dy);
int RectWidth(RectPtr r);
int RectHeight(RectPtr r);

