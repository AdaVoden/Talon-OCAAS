#include <iostream.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include "ClientChannel.H"
#include "EncodeBuffer.H"
#include "DecodeBuffer.H"
#include "util.H"



ClientChannel::ClientChannel(int xClientFD, unsigned int statisticsLevel):
  readBuffer_(xClientFD, this),
  fd_(xClientFD),
  firstRequest_(1),
  firstReply_(1),
  statisticsLevel_(statisticsLevel)
{
}


ClientChannel::~ClientChannel()
{
  if (statisticsLevel_ > 0)
  {
    *logofs << "\n*** dxpc Client-side Compression Statistics ***\n";
    if (statisticsLevel_ >= 2)
      *logofs << "\nCompression of requests by message type:\n";
    unsigned int bitsIn, bitsOut;
    stats_.summarize(bitsIn, bitsOut, (statisticsLevel_ >= 2));

    if (statisticsLevel_ >= 2)
      *logofs << '\n' << framingBitsOut_ <<
	" bits used for dxpc message framing and multiplexing\n";
    bitsOut += framingBitsOut_;

    *logofs << "\nOverall compression:" << endl << "  " <<
      bitsIn << " bits compressed to " << bitsOut << endl;
    if (bitsOut > 0)
      *logofs << "  (" << (float) bitsIn / (float) bitsOut <<
	":1 compression ratio)" << endl << endl;
  }
}


int
ClientChannel::doRead(EncodeBuffer& encodeBuffer)
{
  if (!readBuffer_.doRead())
    return 0;

  const unsigned char *buffer;
  unsigned int size;
  while ((buffer = readBuffer_.getMessage(size)) != 0)
  {
    if (firstRequest_)
    {
      for (unsigned int i = 0; i < size; i++)
	encodeBuffer.encodeValue((unsigned int) buffer[i], 8);
      firstRequest_ = 0;
    }
    else
    {
      clientCache_.lastRequestSequenceNum++;
      unsigned char opcode = *buffer;
      if ((opcode == X_PolyFillRectangle) &&
	  (GetUINT(buffer + 2, bigEndian_) == 3))
	opcode = X_NoOperation;
      encodeBuffer.encodeCachedValue(opcode, 8,
			 clientCache_.opcodeCache[clientCache_.lastOpcode]);
      clientCache_.lastOpcode = opcode;

      switch (opcode)
      {
      case X_AllocColor:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
					 clientCache_.colormapCache, 9);
	  const unsigned char *nextSrc = buffer + 8;
	  unsigned int colorData[3];
	  for (unsigned int i = 0; i < 3; i++)
	  {
	    unsigned int value = GetUINT(nextSrc, bigEndian_);
	    encodeBuffer.encodeCachedValue(value, 16,
				  *(clientCache_.allocColorRGBCache[i]), 4);
	    colorData[i] = value;
	    nextSrc += 2;
	  }
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum, opcode,
				 colorData[0], colorData[1], colorData[2]);
	}
	break;
      case X_ChangeProperty:
	{
	  unsigned char format = buffer[16];
	  encodeBuffer.encodeCachedValue(format, 8,
				    clientCache_.changePropertyFormatCache);
	  unsigned int dataLength = GetULONG(buffer + 20, bigEndian_);
	  encodeBuffer.encodeValue(dataLength, 32, 6);
	  encodeBuffer.encodeValue(buffer[1], 2);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
						  bigEndian_), 29,
			       clientCache_.changePropertyPropertyCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 12,
						  bigEndian_), 29,
				   clientCache_.changePropertyTypeCache, 9);
	  const unsigned char *nextSrc = buffer + 24;
	  if (format == 8)
	  {
	    clientCache_.changePropertyTextCompressor.reset();
	    for (unsigned int i = 0; i < dataLength; i++)
	      clientCache_.changePropertyTextCompressor.encodeChar
		(*nextSrc++, encodeBuffer);
	  }
	  else if (format == 32)
	  {
	    for (unsigned int i = 0; i < dataLength; i++)
	    {
	      encodeBuffer.encodeCachedValue(GetULONG(nextSrc,
						      bigEndian_), 32,
				    clientCache_.changePropertyData32Cache);
	      nextSrc += 4;
	    }
	  }
	  else
	  {
	    for (unsigned int i = 0; i < dataLength; i++)
	    {
	      encodeBuffer.encodeValue(GetUINT(nextSrc,
					       bigEndian_), 16);
	      nextSrc += 2;
	    }
	  }
	}
	break;
      case X_ChangeWindowAttributes:
	{
	  encodeBuffer.encodeValue((size - 12) >> 2, 4);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  unsigned int bitmask = GetULONG(buffer + 8, bigEndian_);
	  encodeBuffer.encodeCachedValue(bitmask, 15,
				     clientCache_.createWindowBitmaskCache);
	  const unsigned char *nextSrc = buffer + 12;
	  unsigned int mask = 0x1;
	  for (unsigned int j = 0; j < 15; j++)
	  {
	    if (bitmask & mask)
	    {
	      encodeBuffer.encodeCachedValue(GetULONG(nextSrc,
						      bigEndian_), 32,
				    *clientCache_.createWindowAttrCache[j]);
	      nextSrc += 4;
	    }
	    mask <<= 1;
	  }
	}
	break;
      case X_ClearArea:
	{
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 1);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  const unsigned char *nextSrc = buffer + 8;
	  for (unsigned int i = 0; i < 4; i++)
	  {
	    encodeBuffer.encodeCachedValue(GetUINT(nextSrc,
						   bigEndian_), 16,
				    *clientCache_.clearAreaGeomCache[i], 8);
	    nextSrc += 2;
	  }
	}
	break;
      case X_CloseFont:
	{
	  unsigned int font = GetULONG(buffer + 4, bigEndian_);
	  encodeBuffer.encodeValue(font - clientCache_.lastFont,
				   29, 5);
	  clientCache_.lastFont = font;
	}
	break;
      case X_ConfigureWindow:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  unsigned int bitmask = GetUINT(buffer + 8, bigEndian_);
	  encodeBuffer.encodeCachedValue(bitmask, 7,
				  clientCache_.configureWindowBitmaskCache);
	  unsigned int mask = 0x1;
	  const unsigned char *nextSrc = buffer + 12;
	  for (unsigned int i = 0; i < 7; i++)
	  {
	    if (bitmask & mask)
	    {
	      encodeBuffer.encodeCachedValue(GetULONG(nextSrc,
				bigEndian_), CONFIGUREWINDOW_FIELD_WIDTH[i],
				  *clientCache_.configureWindowAttrCache[i],
					     8);
	      nextSrc += 4;
	    }
	    mask <<= 1;
	  }
	}
	break;
      case X_ConvertSelection:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
			clientCache_.convertSelectionRequestorCache, 9);
	  const unsigned char* nextSrc = buffer + 8;
	  for (unsigned int i = 0; i < 3; i++)
	  {
	    encodeBuffer.encodeCachedValue(GetULONG(nextSrc, bigEndian_), 29,
			*(clientCache_.convertSelectionAtomCache[i]), 9);
	    nextSrc += 4;
	  }
	  unsigned int timestamp = GetULONG(nextSrc, bigEndian_);
	  encodeBuffer.encodeValue(timestamp -
		     clientCache_.convertSelectionLastTimestamp, 32, 4);
	  clientCache_.convertSelectionLastTimestamp = timestamp;
	}
	break;
      case X_CopyArea:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 12,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  const unsigned char *nextSrc = buffer + 16;
	  for (unsigned int i = 0; i < 6; i++)
	  {
	    encodeBuffer.encodeCachedValue(GetUINT(nextSrc,
						   bigEndian_), 16,
				     *clientCache_.copyAreaGeomCache[i], 8);
	    nextSrc += 2;
	  }
	}
	break;
      case X_CopyGC:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 12,
			bigEndian_), 23, clientCache_.createGCBitmaskCache);
	}
	break;
      case X_CopyPlane:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 12,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  const unsigned char *nextSrc = buffer + 16;
	  for (unsigned int i = 0; i < 6; i++)
	  {
	    encodeBuffer.encodeCachedValue(GetUINT(nextSrc,
						   bigEndian_), 16,
				    *clientCache_.copyPlaneGeomCache[i], 8);
	    nextSrc += 2;
	  }
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 28,
						  bigEndian_), 32,
				   clientCache_.copyPlaneBitPlaneCache, 10);
	}
	break;
      case X_CreateGC:
      case X_ChangeGC:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  const unsigned char *nextSrc = buffer + 8;
	  if (opcode == X_CreateGC)
	  {
	    encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	    nextSrc += 4;
	  }
	  unsigned int bitmask = GetULONG(nextSrc, bigEndian_);
	  nextSrc += 4;
	  encodeBuffer.encodeCachedValue(bitmask, 23,
					 clientCache_.createGCBitmaskCache);
	  unsigned int mask = 0x1;
	  for (unsigned int i = 0; i < 23; i++)
	  {
	    if (bitmask & mask)
	    {
	      unsigned int value = GetULONG(nextSrc, bigEndian_);
	      nextSrc += 4;
	      unsigned int fieldWidth = CREATEGC_FIELD_WIDTH[i];
	      if (fieldWidth <= 4)
	      {
		encodeBuffer.encodeValue(value, fieldWidth);
	      }
	      else
	      {
		encodeBuffer.encodeCachedValue(value,
					       fieldWidth,
					*clientCache_.createGCAttrCache[i]);
	      }
	    }
	    mask <<= 1;
	  }
	}
	break;
      case X_CreatePixmap:
	{
	  encodeBuffer.encodeCachedValue(buffer[1], 8,
					 clientCache_.depthCache);
	  unsigned int pixmap = GetULONG(buffer + 4, bigEndian_);
	  unsigned int diff = pixmap -
	  clientCache_.createPixmapLastPixmap;
	  if (diff == 0)
	    encodeBuffer.encodeValue(1, 1);
	  else
	  {
	    encodeBuffer.encodeValue(0, 1);
	    encodeBuffer.encodeValue(diff, 29, 4);
	    clientCache_.createPixmapLastPixmap = pixmap;
	  }
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetUINT(buffer + 12,
		       bigEndian_), 16, clientCache_.createPixmapXCache, 8);
	  encodeBuffer.encodeCachedValue(GetUINT(buffer + 14,
		       bigEndian_), 16, clientCache_.createPixmapYCache, 8);
	}
	break;
      case X_CreateWindow:
	{
	  encodeBuffer.encodeCachedValue((unsigned int) buffer[1], 8,
					 clientCache_.depthCache);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  const unsigned char *nextSrc = buffer + 12;
	  for (unsigned int i = 0; i < 6; i++)
	  {
	    encodeBuffer.encodeCachedValue(GetUINT(nextSrc,
						   bigEndian_), 16,
				 *clientCache_.createWindowGeomCache[i], 8);
	    nextSrc += 2;
	  }
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 24,
				 bigEndian_), 29, clientCache_.visualCache);
	  unsigned int bitmask = GetULONG(buffer + 28, bigEndian_);
	  encodeBuffer.encodeCachedValue(bitmask, 15,
				     clientCache_.createWindowBitmaskCache);
	  nextSrc = buffer + 32;
	  unsigned int mask = 0x1;
	  for (unsigned int j = 0; j < 15; j++)
	  {
	    if (bitmask & mask)
	    {
	      encodeBuffer.encodeCachedValue(GetULONG(nextSrc,
						      bigEndian_), 32,
				    *clientCache_.createWindowAttrCache[j]);
	      nextSrc += 4;
	    }
	    mask <<= 1;
	  }
	}
	break;
      case X_DeleteProperty:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
					 clientCache_.windowCache, 9);
	  encodeBuffer.encodeValue(GetULONG(buffer + 8, bigEndian_), 29, 9);
	}
	break;
      case X_FillPoly:
	{
	  unsigned int numPoints = ((size - 16) >> 2);
	  encodeBuffer.encodeCachedValue(numPoints, 14,
				 clientCache_.fillPolyNumPointsCache, 4);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  encodeBuffer.encodeValue((unsigned int) buffer[12], 2);
	  encodeBuffer.encodeValue((unsigned int) buffer[13], 1);
	  int relativeCoordMode = (buffer[13] != 0);
	  const unsigned char *nextSrc = buffer + 16;
	  unsigned int pointIndex = 0;
	  for (unsigned int i = 0; i < numPoints; i++)
	  {
	    if (relativeCoordMode)
	    {
	      encodeBuffer.encodeCachedValue(GetUINT(nextSrc,
						     bigEndian_), 16,
				*clientCache_.fillPolyXRelCache[pointIndex],
					     8);
	      nextSrc += 2;
	      encodeBuffer.encodeCachedValue(GetUINT(nextSrc,
						     bigEndian_), 16,
				*clientCache_.fillPolyYRelCache[pointIndex],
					     8);
	      nextSrc += 2;
	    }
	    else
	    {
	      unsigned int x = GetUINT(nextSrc, bigEndian_);
	      nextSrc += 2;
	      unsigned int y = GetUINT(nextSrc, bigEndian_);
	      nextSrc += 2;
	      unsigned int j;
	      for (j = 0; j < 8; j++)
		if ((x == clientCache_.fillPolyRecentX[j]) &&
		    (y == clientCache_.fillPolyRecentY[j]))
		  break;
	      if (j < 8)
	      {
		encodeBuffer.encodeValue(1, 1);
		encodeBuffer.encodeValue(j, 3);
	      }
	      else
	      {
		encodeBuffer.encodeValue(0, 1);
		encodeBuffer.encodeCachedValue(x, 16,
				*clientCache_.fillPolyXAbsCache[pointIndex],
					       8);
		encodeBuffer.encodeCachedValue(y, 16,
				*clientCache_.fillPolyYAbsCache[pointIndex],
					       8);
		clientCache_.fillPolyRecentX[clientCache_.fillPolyIndex] = x;
		clientCache_.fillPolyRecentY[clientCache_.fillPolyIndex] = y;
		clientCache_.fillPolyIndex++;
		if (clientCache_.fillPolyIndex == 8)
		  clientCache_.fillPolyIndex = 0;
	      }
	    }
	    if (pointIndex + 1 < FILL_POLY_MAX_POINTS)
	      pointIndex++;
	  }
	}
	break;
      case X_FreeColors:
	{
	  unsigned int numPixels = GetUINT(buffer + 2, bigEndian_) - 3;
	  encodeBuffer.encodeValue(numPixels, 16, 4);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
					 clientCache_.colormapCache, 9);
	  encodeBuffer.encodeValue(GetULONG(buffer + 8, bigEndian_), 32, 4);
	  const unsigned char *nextSrc = buffer + 12;
	  while (numPixels)
	  {
	    encodeBuffer.encodeValue(GetULONG(nextSrc, bigEndian_), 32, 8);
	    nextSrc += 4;
	    numPixels--;
	  }
	}
	break;
      case X_FreeCursor:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_),
					 29, clientCache_.cursorCache, 9);
	}
	break;
      case X_FreeGC:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	}
	break;
      case X_FreePixmap:
	{
	  unsigned int pixmap = GetULONG(buffer + 4, bigEndian_);
	  unsigned int diff = pixmap -
	  clientCache_.createPixmapLastPixmap;
	  if (diff == 0)
	    encodeBuffer.encodeValue(1, 1);
	  else
	  {
	    encodeBuffer.encodeValue(0, 1);
	    clientCache_.createPixmapLastPixmap = pixmap;
	    encodeBuffer.encodeValue(diff, 29, 4);
	  }
	}
	break;
      case X_GetAtomName:
	{
	  encodeBuffer.encodeValue(GetULONG(buffer + 4, bigEndian_), 29, 9);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_GetGeometry:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_GetInputFocus:
      case X_GetModifierMapping:
	{
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_GetKeyboardMapping:
	{
	  encodeBuffer.encodeValue((unsigned int) buffer[4], 8);
	  encodeBuffer.encodeValue((unsigned int) buffer[5], 8);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_GetProperty:
	{
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 1);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
						  bigEndian_), 29,
					 clientCache_.windowCache, 9);
	  unsigned int property = GetULONG(buffer + 8, bigEndian_);
	  encodeBuffer.encodeValue(property, 29, 9);
	  encodeBuffer.encodeValue(GetULONG(buffer + 12, bigEndian_),
				   29, 9);
	  encodeBuffer.encodeValue(GetULONG(buffer + 16, bigEndian_),
				   32, 2);
	  encodeBuffer.encodeValue(GetULONG(buffer + 20, bigEndian_),
				   32, 8);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode, property);
	}
	break;
      case X_GetSelectionOwner:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
						  bigEndian_), 29,
			   clientCache_.getSelectionOwnerSelectionCache, 9);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_GrabButton:
      case X_GrabPointer:
	{
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 1);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  encodeBuffer.encodeCachedValue(GetUINT(buffer + 8,
						 bigEndian_), 16,
				     clientCache_.grabButtonEventMaskCache);
	  encodeBuffer.encodeValue((unsigned int) buffer[10], 1);
	  encodeBuffer.encodeValue((unsigned int) buffer[11], 1);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 12,
						  bigEndian_), 29,
				    clientCache_.grabButtonConfineCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 16,
			      bigEndian_), 29, clientCache_.cursorCache, 9);
	  if (opcode == X_GrabButton)
	  {
	    encodeBuffer.encodeCachedValue(buffer[20], 8,
					clientCache_.grabButtonButtonCache);
	    encodeBuffer.encodeCachedValue(GetUINT(buffer + 22,
						   bigEndian_), 16,
				      clientCache_.grabButtonModifierCache);
	  }
	  else
	  {
	    unsigned int timestamp = GetULONG(buffer + 20,
					      bigEndian_);
	    encodeBuffer.encodeValue(timestamp -
			     clientCache_.grabKeyboardLastTimestamp, 32, 4);
	    clientCache_.grabKeyboardLastTimestamp = timestamp;
	    sequenceNumQueue_.push(
			       clientCache_.lastRequestSequenceNum, opcode);
	  }
	}
	break;
      case X_GrabKeyboard:
	{
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 1);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  unsigned int timestamp = GetULONG(buffer + 8, bigEndian_);
	  encodeBuffer.encodeValue(timestamp -
			     clientCache_.grabKeyboardLastTimestamp, 32, 4);
	  clientCache_.grabKeyboardLastTimestamp = timestamp;
	  encodeBuffer.encodeValue((unsigned int) buffer[12], 1);
	  encodeBuffer.encodeValue((unsigned int) buffer[13], 1);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_GrabServer:
      case X_UngrabServer:
      case X_NoOperation:
	{
	}
	break;
      case X_ImageText8:
	{
	  unsigned int textLength = (unsigned int) buffer[1];
	  encodeBuffer.encodeValue(textLength, 8);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  unsigned int x = GetUINT(buffer + 12, bigEndian_);
	  int xDiff = x - clientCache_.imageText8LastX;
	  clientCache_.imageText8LastX = x;
	  encodeBuffer.encodeCachedValue(xDiff, 16,
					 clientCache_.imageText8CacheX, 8);
	  unsigned int y = GetUINT(buffer + 14, bigEndian_);
	  int yDiff = y - clientCache_.imageText8LastY;
	  clientCache_.imageText8LastY = y;
	  encodeBuffer.encodeCachedValue(yDiff, 16,
					 clientCache_.imageText8CacheY, 8);
	  const unsigned char *nextSrc = buffer + 16;
	  clientCache_.imageText8TextCompressor.reset();
	  for (unsigned int j = 0; j < textLength; j++)
	    clientCache_.imageText8TextCompressor.encodeChar(
						  *nextSrc++, encodeBuffer);
	}
	break;
      case X_InternAtom:
	{
	  unsigned int nameLength = GetUINT(buffer + 4, bigEndian_);
	  encodeBuffer.encodeValue(nameLength, 16, 6);
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 1);
	  const unsigned char *nextSrc = buffer + 8;
	  clientCache_.internAtomTextCompressor.reset();
	  for (unsigned int i = 0; i < nameLength; i++)
	    clientCache_.internAtomTextCompressor.encodeChar(
						  *nextSrc++, encodeBuffer);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_ListExtensions:
	{
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_ListFonts:
	{
	  unsigned int textLength = GetUINT(buffer + 6, bigEndian_);
	  encodeBuffer.encodeValue(textLength, 16, 6);
	  encodeBuffer.encodeValue(GetUINT(buffer + 4, bigEndian_), 16, 6);
	  const unsigned char* nextSrc = buffer + 8;
	  clientCache_.polyText8TextCompressor.reset();
	  for (unsigned int i = 0; i < textLength; i++)
	    clientCache_.polyText8TextCompressor.encodeChar(
						  *nextSrc++, encodeBuffer);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	}
	break;
      case X_LookupColor:
      case X_AllocNamedColor:
	{
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum,
				 opcode);
	  unsigned int textLength = GetUINT(buffer + 8, bigEndian_);
	  encodeBuffer.encodeValue(textLength, 16, 6);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_),
					 29, clientCache_.colormapCache, 9);
	  const unsigned char *nextSrc = buffer + 12;
	  clientCache_.polyText8TextCompressor.reset();
	  for (unsigned int i = 0; i < textLength; i++)
	    clientCache_.polyText8TextCompressor.encodeChar(
						  *nextSrc++, encodeBuffer);
	}
	break;
      case X_MapWindow:
      case X_UnmapWindow:
      case X_MapSubwindows:
      case X_GetWindowAttributes:
      case X_DestroyWindow:
      case X_DestroySubwindows:
      case X_QueryPointer:
      case X_QueryTree:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			      bigEndian_), 29, clientCache_.windowCache, 9);
	  if ((opcode == X_QueryPointer) ||
	      (opcode == X_GetWindowAttributes) ||
	      (opcode == X_QueryTree))
	  {
	    sequenceNumQueue_.push(
			       clientCache_.lastRequestSequenceNum, opcode);
	  }
	}
	break;
      case X_OpenFont:
	{
	  unsigned int nameLength = GetUINT(buffer + 8, bigEndian_);
	  encodeBuffer.encodeValue(nameLength, 16, 7);
	  unsigned int font = GetULONG(buffer + 4, bigEndian_);
	  encodeBuffer.encodeValue(font - clientCache_.lastFont,
				   29, 5);
	  clientCache_.lastFont = font;
	  const unsigned char *nextSrc = buffer + 12;
	  clientCache_.openFontTextCompressor.reset();
	  for (; nameLength; nameLength--)
	    clientCache_.openFontTextCompressor.encodeChar(
						  *nextSrc++, encodeBuffer);
	}
	break;
      case X_PolyFillRectangle:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  unsigned int index = 0;
	  unsigned int lastX = 0, lastY = 0;
	  unsigned int lastWidth = 0, lastHeight = 0;
	  for (unsigned int i = 12; i < size;)
	  {
	    unsigned int x = GetUINT(buffer + i, bigEndian_);
	    unsigned int newX = x;
	    x -= lastX;
	    lastX = newX;
	    encodeBuffer.encodeCachedValue(x, 16,
			   *clientCache_.polyFillRectangleCacheX[index], 8);
	    i += 2;
	    unsigned int y = GetUINT(buffer + i, bigEndian_);
	    unsigned int newY = y;
	    y -= lastY;
	    lastY = newY;
	    encodeBuffer.encodeCachedValue(y, 16,
			   *clientCache_.polyFillRectangleCacheY[index], 8);
	    i += 2;
	    unsigned int width = GetUINT(buffer + i, bigEndian_);
	    unsigned int newWidth = width;
	    width -= lastWidth;
	    lastWidth = newWidth;
	    encodeBuffer.encodeCachedValue(width, 16,
			   *clientCache_.polyFillRectangleCacheWidth[index],
					   8);
	    i += 2;
	    unsigned int height = GetUINT(buffer + i, bigEndian_);
	    unsigned int newHeight = height;
	    height -= lastHeight;
	    lastHeight = newHeight;
	    encodeBuffer.encodeCachedValue(height, 16,
			  *clientCache_.polyFillRectangleCacheHeight[index],
					   8);
	    i += 2;
	    index = 1;

	    encodeBuffer.encodeValue(((i < size) ? 1 : 0), 1);
	  }
	}
	break;
      case X_PolyPoint:
	{
	  encodeBuffer.encodeValue(GetUINT(buffer + 2, bigEndian_) - 3,
				   16, 4);
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 1);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  const unsigned char *nextSrc = buffer + 12;
	  unsigned int index = 0;
	  unsigned int lastX = 0, lastY = 0;
	  for (unsigned int i = 12; i < size; i += 4)
	  {
	    unsigned int x = GetUINT(nextSrc, bigEndian_);
	    nextSrc += 2;
	    unsigned int tmp = x;
	    x -= lastX;
	    lastX = tmp;
	    encodeBuffer.encodeCachedValue(x, 16,
				   *clientCache_.polyPointCacheX[index], 8);
	    unsigned int y = GetUINT(nextSrc, bigEndian_);
	    nextSrc += 2;
	    tmp = y;
	    y -= lastY;
	    lastY = tmp;
	    encodeBuffer.encodeCachedValue(y, 16,
				   *clientCache_.polyPointCacheY[index], 8);
	    index = 1;
	  }
	}
	break;
      case X_PolyLine:
	{
	  encodeBuffer.encodeValue(GetUINT(buffer + 2, bigEndian_) - 3,
				   16, 4);
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 1);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  const unsigned char *nextSrc = buffer + 12;
	  unsigned int index = 0;
	  unsigned int lastX = 0, lastY = 0;
	  for (unsigned int i = 12; i < size; i += 4)
	  {
	    unsigned int x = GetUINT(nextSrc, bigEndian_);
	    nextSrc += 2;
	    unsigned int tmp = x;
	    x -= lastX;
	    lastX = tmp;
	    encodeBuffer.encodeCachedValue(x, 16,
				    *clientCache_.polyLineCacheX[index], 8);
	    unsigned int y = GetUINT(nextSrc, bigEndian_);
	    nextSrc += 2;
	    tmp = y;
	    y -= lastY;
	    lastY = tmp;
	    encodeBuffer.encodeCachedValue(y, 16,
				    *clientCache_.polyLineCacheY[index], 8);
	    index = 1;
	  }
	}
	break;
      case X_PolyRectangle:
	{
	  encodeBuffer.encodeValue((GetUINT(buffer + 2,
					    bigEndian_) - 3) >> 1, 16, 3);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  const unsigned char *end = buffer + size;
	  const unsigned char *nextSrc = buffer + 12;
	  while (nextSrc < end)
	    for (unsigned int i = 0; i < 4; i++)
	    {
	      encodeBuffer.encodeCachedValue(GetUINT(nextSrc,
						     bigEndian_), 16,
				*clientCache_.polyRectangleGeomCache[i], 8);
	      nextSrc += 2;
	    }
	}
	break;
      case X_PolySegment:
	{
	  encodeBuffer.encodeValue((GetUINT(buffer + 2,
					    bigEndian_) - 3) >> 1, 16, 4);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  const unsigned char *end = buffer + size;
	  const unsigned char *nextSrc = buffer + 12;
	  // unsigned int index = 0;
	  // unsigned int lastX1, lastY1, lastX2, lastY2;
	  while (nextSrc < end)
	  {
	    unsigned int x = GetUINT(nextSrc, bigEndian_);
	    nextSrc += 2;
	    unsigned int xDiff0 =
	    x - clientCache_.polySegmentLastX[0];
	    unsigned int xDiff1 =
	    x - clientCache_.polySegmentLastX[1];
	    int xDiff0Abs = (int) xDiff0;
	    if (xDiff0Abs < 0)
	      xDiff0Abs = -xDiff0Abs;
	    int xDiff1Abs = (int) xDiff1;
	    if (xDiff1Abs < 0)
	      xDiff1Abs = -xDiff1Abs;

	    unsigned int y = GetUINT(nextSrc, bigEndian_);
	    nextSrc += 2;
	    unsigned int yDiff0 =
	    y - clientCache_.polySegmentLastY[0];
	    unsigned int yDiff1 =
	    y - clientCache_.polySegmentLastY[1];
	    int yDiff0Abs = (int) yDiff0;
	    if (yDiff0Abs < 0)
	      yDiff0Abs = -yDiff0Abs;
	    int yDiff1Abs = (int) yDiff1;
	    if (yDiff1Abs < 0)
	      yDiff1Abs = -yDiff1Abs;

	    int diff0 = xDiff0Abs + yDiff0Abs;
	    int diff1 = xDiff1Abs + yDiff1Abs;
	    if (diff0 < diff1)
	    {
	      encodeBuffer.encodeValue(0, 1);
	      encodeBuffer.encodeCachedValue(xDiff0, 16,
					 clientCache_.polySegmentCacheX, 6);
	      encodeBuffer.encodeCachedValue(yDiff0, 16,
					 clientCache_.polySegmentCacheY, 6);
	    }
	    else
	    {
	      encodeBuffer.encodeValue(1, 1);
	      encodeBuffer.encodeCachedValue(xDiff1, 16,
					 clientCache_.polySegmentCacheX, 6);
	      encodeBuffer.encodeCachedValue(yDiff1, 16,
					 clientCache_.polySegmentCacheY, 6);
	    }

	    clientCache_.polySegmentLastX[clientCache_.polySegmentCacheIndex] = x;
	    clientCache_.polySegmentLastY[clientCache_.polySegmentCacheIndex] = y;

	    clientCache_.polySegmentCacheIndex =
	      clientCache_.polySegmentCacheIndex == 1 ? 0 : 1;
	  }
	}
	break;
      case X_PolyText8:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  unsigned int x = GetUINT(buffer + 12, bigEndian_);
	  int xDiff = x - clientCache_.polyText8LastX;
	  clientCache_.polyText8LastX = x;
	  encodeBuffer.encodeCachedValue(xDiff, 16,
					 clientCache_.polyText8CacheX, 8);
	  unsigned int y = GetUINT(buffer + 14, bigEndian_);
	  int yDiff = y - clientCache_.polyText8LastY;
	  clientCache_.polyText8LastY = y;
	  encodeBuffer.encodeCachedValue(yDiff, 16,
					 clientCache_.polyText8CacheY, 8);
	  const unsigned char *end = buffer + size - 1;
	  const unsigned char *nextSrc = buffer + 16;
	  while (nextSrc < end)
	  {
	    unsigned int textLength = (unsigned int) *nextSrc++;
	    encodeBuffer.encodeValue(1, 1);
	    encodeBuffer.encodeValue(textLength, 8);
	    if (textLength == 255)
	    {
	      encodeBuffer.encodeCachedValue(GetULONG(nextSrc, 1),
				       29, clientCache_.polyText8FontCache);
	      nextSrc += 4;
	    }
	    else
	    {
	      encodeBuffer.encodeCachedValue(*nextSrc++,
				       8, clientCache_.polyText8DeltaCache);
	      clientCache_.polyText8TextCompressor.reset();
	      for (unsigned int i = 0; i < textLength; i++)
		clientCache_.polyText8TextCompressor.encodeChar(
						  *nextSrc++, encodeBuffer);
	    }
	  }
	  encodeBuffer.encodeValue(0, 1);
	}
	break;
      case X_PutImage:
	{
	  encodeBuffer.encodeValue(GetUINT(buffer + 2, bigEndian_),
				   16, 8);
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 2);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8,
				  bigEndian_), 29, clientCache_.gcCache, 9);
	  unsigned int width = GetUINT(buffer + 12, bigEndian_);
	  encodeBuffer.encodeCachedValue(width, 16,
					 clientCache_.putImageWidthCache, 8);
	  unsigned int height = GetUINT(buffer + 14, bigEndian_);
	  encodeBuffer.encodeCachedValue(height, 16,
				       clientCache_.putImageHeightCache, 8);
	  unsigned int x = GetUINT(buffer + 16, bigEndian_);
	  int xDiff = x - clientCache_.putImageLastX;
	  clientCache_.putImageLastX = x;
	  encodeBuffer.encodeCachedValue(xDiff, 16,
					 clientCache_.putImageXCache, 8);
	  unsigned int y = GetUINT(buffer + 18, bigEndian_);
	  int yDiff = y - clientCache_.putImageLastY;
	  clientCache_.putImageLastY = y;
	  encodeBuffer.encodeCachedValue(yDiff, 16,
					 clientCache_.putImageYCache, 8);
	  encodeBuffer.encodeCachedValue(buffer[20], 8,
					 clientCache_.putImageOffsetCache);
	  encodeBuffer.encodeCachedValue(buffer[21], 8,
					 clientCache_.depthCache);
	  const unsigned char *nextSrc = buffer + 24;
	  if ((buffer[1] == 0) && (height <= 32) &&
	      (width > height * PUT_IMAGE_MIN_ASPECT_RATIO))
	  {
	    // bitmap that probably contains text;
	    // encode using a variant of
	    // text-compression algorithm
	    if ((imageByteOrder_ == 0) && (bitmapBitOrder_ == 0))
	    {
	      unsigned char *next = (unsigned char *) buffer + 24;
	      for (unsigned int i = 24; i < size; i++)
	      {
		*next = REVERSED_BYTE[*next];
		next++;
	      }
	    }
	    unsigned int widthInBits =
	    ((width / scanlinePad_) * scanlinePad_);
	    if (widthInBits < width)
	      widthInBits += scanlinePad_;
	    unsigned int widthInBytes = (widthInBits >> 3);
	    const unsigned char *nextSrc = buffer + 24;
	    unsigned char srcMask = 0x80;
	    clientCache_.putImageLastPixels.reset();
	    for (unsigned int xCoord = 0; xCoord < width; xCoord++)
	    {
	      unsigned int columnValue = 0;
	      const unsigned char *next = nextSrc;
	      for (unsigned int h = 0; h < height; h++)
	      {
		columnValue <<= 1;
		if (srcMask & *next)
		  columnValue |= 1;
		next += widthInBytes;
	      }
	      unsigned int modelNum =
	      clientCache_.putImageLastPixels.getValue();
	      encodeBuffer.encodeCachedValue(columnValue, height,
				  clientCache_.putImagePixelCache[modelNum %
						PUT_IMAGE_PIXEL_CACHE_SIZE],
					     clientCache_.columnPixel0Coder,
					     clientCache_.columnPixel1Coder);
	      clientCache_.putImageLastPixels.add(columnValue);
	      srcMask >>= 1;
	      if (srcMask == 0)
	      {
		srcMask = 0x80;
		nextSrc++;
	      }
	    }
	  }
	  else if (buffer[1] == 0)
	  {
	    // bitmap--use "Modified-Modified-Read" FAX coding
	    if (width + 2 > clientCache_.putImageLineSize)
	    {
	      delete[]clientCache_.putImageReferenceLine;
	      delete[]clientCache_.putImageCodingLine;
	      clientCache_.putImageLineSize = width + 2;
	      clientCache_.putImageReferenceLine =
		new unsigned int[width + 2];
	      clientCache_.putImageCodingLine =
		new unsigned int[width + 2];
	    }
	    unsigned int widthInBits =
	    ((width / scanlinePad_) * scanlinePad_);
	    if (widthInBits < width)
	      widthInBits += scanlinePad_;
	    unsigned int widthInBytes = (widthInBits >> 3);
	    for (unsigned int h = 0; h < height; h++)
	    {
	      unsigned int codingLineLength = 0;
	      const unsigned char *nextSrc =
	      buffer + 24 + h * widthInBytes;
	      unsigned char nextSrcChar = *nextSrc;
	      if (h)
		nextSrcChar ^= *(nextSrc - widthInBytes);
	      if ((imageByteOrder_ == 0) && (bitmapBitOrder_ == 0))
		nextSrcChar = REVERSED_BYTE[nextSrcChar];

	      unsigned char srcMask = 0x80;
	      // precede each encoded row with a single bit
	      // indicating
	      // what pixel value (0 or 1) begins the row.
	      unsigned int lastPixelValue = 0;
	      unsigned int pixelValue =
	      ((nextSrcChar & srcMask) ? 1 : 0);
	      if (h == 0)
		encodeBuffer.encodeValue(pixelValue, 1);
	      for (unsigned int xCoord = 0; xCoord < width;)
	      {
		// right here we're looking at the start of
		// a new run
		unsigned int runStart = xCoord;
		if (pixelValue)
		{
		  if (pixelValue != lastPixelValue)
		    clientCache_.putImageCodingLine[codingLineLength++] = xCoord;
		  while (xCoord < width)
		  {
		    if (!(nextSrcChar & srcMask))
		      break;
		    srcMask >>= 1;
		    if (srcMask == 0)
		    {
		      srcMask = 0x80;
		      nextSrc++;
		      if (xCoord + 1 < width)
		      {
			nextSrcChar = *nextSrc;
			if (h)
			  nextSrcChar ^=
			    *(nextSrc - widthInBytes);
			if ((imageByteOrder_ == 0) &&
			    (bitmapBitOrder_ == 0))
			  nextSrcChar =
			    REVERSED_BYTE[nextSrcChar];
		      }
		    }
		    xCoord++;
		  }
		  lastPixelValue = pixelValue;
		}
		else		// pixelValue == 0

		{
		  if (pixelValue != lastPixelValue)
		    clientCache_.putImageCodingLine[codingLineLength++] = xCoord;
		  while (xCoord < width)
		  {
		    if (nextSrcChar & srcMask)
		      break;
		    srcMask >>= 1;
		    if (srcMask == 0)
		    {
		      srcMask = 0x80;
		      nextSrc++;
		      if (xCoord + 1 < width)
		      {
			nextSrcChar = *nextSrc;
			if (h)
			  nextSrcChar ^= *(nextSrc - widthInBytes);
			if ((imageByteOrder_ == 0) && (bitmapBitOrder_ == 0))
			  nextSrcChar = REVERSED_BYTE[nextSrcChar];
		      }
		    }
		    xCoord++;
		  }
		  lastPixelValue = pixelValue;
		}
		// here 'nextSrc' points to either a color change or the
		// pixel after the end of the scan line.  Thus the length
		// of the solid-color block that just ended can be encoded
		// (invariant: runLength >= 1)
		unsigned int runLength = xCoord - runStart;
		if (pixelValue)
		{
		  if (h == 0)
		    clientCache_.putImagePixel1Coder.encode(runLength,
							    encodeBuffer);
		  pixelValue = 0;
		}
		else
		{
		  if (h == 0)
		    clientCache_.putImagePixel0Coder.encode(runLength,
							    encodeBuffer);
		  pixelValue = 1;
		}
	      }
	      clientCache_.putImageCodingLine[codingLineLength++] = width;
	      clientCache_.putImageCodingLine[codingLineLength++] = width;

	      if (h)
	      {
		unsigned int lastX = 0;
		unsigned int nextCodingIndex = 0, nextReferenceIndex = 0;
		while (lastX < width)
		{
		  unsigned int nextCoding =
		  clientCache_.putImageCodingLine[nextCodingIndex];
		  unsigned int nextReference =
		  clientCache_.putImageReferenceLine[nextReferenceIndex];
		  if (nextCoding == nextReference)
		  {
		    clientCache_.putImageDiffCoder.encode(SD_VERTICAL_0,
							  encodeBuffer);
		    lastX = nextCoding;
		    nextCodingIndex++;
		    nextReferenceIndex++;
		    continue;
		  }
		  if (nextCoding > nextReference)
		  {
		    unsigned int diff = nextCoding - nextReference;
		    if (diff == 1)
		    {
		      clientCache_.putImageDiffCoder.encode(SD_VERTICAL_PLUS_1,
							    encodeBuffer);
		      lastX = nextCoding;
		      nextCodingIndex++;
		      nextReferenceIndex++;
		    }
		    else if (diff == 2)
		    {
		      clientCache_.putImageDiffCoder.encode(SD_VERTICAL_PLUS_2,
							    encodeBuffer);
		      lastX = nextCoding;
		      nextCodingIndex++;
		      nextReferenceIndex++;
		    }
		    else
		    {
		      clientCache_.putImageDiffCoder.encode(SD_PASS,
							    encodeBuffer);
		      nextReferenceIndex += 2;
		    }
		  }
		  else		// (nextCoding < nextReference)

		  {
		    unsigned int diff = nextReference - nextCoding;
		    if (nextReference == width)
		      diff = 99999;
		    if (diff == 1)
		    {
		      clientCache_.putImageDiffCoder.encode(
					 SD_VERTICAL_MINUS_1, encodeBuffer);
		      lastX = nextCoding;
		      nextCodingIndex++;
		      nextReferenceIndex++;
		    }
		    else if (diff == 2)
		    {
		      clientCache_.putImageDiffCoder.encode(
					 SD_VERTICAL_MINUS_2, encodeBuffer);
		      lastX = nextCoding;
		      nextCodingIndex++;
		      nextReferenceIndex++;
		    }
		    else
		    {
		      clientCache_.putImageDiffCoder.encode(SD_HORIZONTAL,
							    encodeBuffer);
		      if (nextCodingIndex & 1)
			clientCache_.putImagePixel0Coder.encode(
					  nextCoding - lastX, encodeBuffer);
		      else
			clientCache_.putImagePixel1Coder.encode(
					  nextCoding - lastX, encodeBuffer);
		      lastX = nextCoding;
		      nextCoding =
			clientCache_.putImageCodingLine[++nextCodingIndex];
		      if (nextCodingIndex & 1)
			clientCache_.putImagePixel0Coder.encode(
					  nextCoding - lastX, encodeBuffer);
		      else
			clientCache_.putImagePixel1Coder.encode(
					  nextCoding - lastX, encodeBuffer);
		      lastX = nextCoding;
		      nextCodingIndex++;
		    }
		  }
		}
	      }
	      unsigned int *tmp = clientCache_.putImageReferenceLine;
	      clientCache_.putImageReferenceLine =
		clientCache_.putImageCodingLine;
	      clientCache_.putImageCodingLine = tmp;
	    }
	  }
	  else
	  {
	    // pixmap, not bitmap
	    if (buffer[21] == 8)
	    {
	      for (unsigned int i = 24; i < size; i++)
		encodeBuffer.encodeCachedValue(*nextSrc++, 8,
					 clientCache_.putImageByteCache, 4);
	    }
	    else
	    {
	      for (unsigned int i = 24; i < size; i++)
		encodeBuffer.encodeValue((unsigned int) *nextSrc++, 8);
	    }
	  }
	}
	break;
      case X_QueryBestSize:
	{
	  encodeBuffer.encodeValue((unsigned int)buffer[1], 2);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4,
			    bigEndian_), 29, clientCache_.drawableCache, 9);
	  encodeBuffer.encodeValue(GetUINT(buffer + 8, bigEndian_), 16, 8);
	  encodeBuffer.encodeValue(GetUINT(buffer + 10, bigEndian_), 16, 8);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum, opcode);
	}
	break;
      case X_QueryColors:
	{
	  unsigned int numColors = ((size - 8) >> 2);
	  encodeBuffer.encodeValue(numColors, 16, 5);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
					 clientCache_.colormapCache, 9);
	  const unsigned char *nextSrc = buffer + 8;
	  unsigned int predictedPixel = clientCache_.queryColorsLastPixel;
	  for (unsigned int i = 0; i < numColors; i++)
	  {
	    unsigned int pixel = GetULONG(nextSrc, bigEndian_);
	    nextSrc += 4;
	    if (pixel == predictedPixel)
	      encodeBuffer.encodeValue(1, 1);
	    else
	    {
	      encodeBuffer.encodeValue(0, 1);
	      encodeBuffer.encodeValue(pixel, 32, 9);
	    }
	    if (i == 0)
	      clientCache_.queryColorsLastPixel = pixel;
	    predictedPixel = pixel + 1;
	  }
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum, opcode);
	}
	break;
      case X_QueryExtension:
	{
	  unsigned int nameLength = GetUINT(buffer + 4, bigEndian_);
	  encodeBuffer.encodeValue(nameLength, 16, 6);
	  const unsigned char *nextSrc = buffer + 8;
	  for (; nameLength; nameLength--)
	    encodeBuffer.encodeValue((unsigned int) *nextSrc++, 8);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum, opcode);
	}
	break;
      case X_QueryFont:
	{
	  unsigned int font = GetULONG(buffer + 4, bigEndian_);
	  encodeBuffer.encodeValue(font - clientCache_.lastFont, 29, 5);
	  clientCache_.lastFont = font;
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum, opcode);
	}
	break;
      case X_SetClipRectangles:
	{
	  unsigned int numRectangles = ((size - 12) >> 3);
	  encodeBuffer.encodeValue(numRectangles, 13, 4);
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 2);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
					 clientCache_.gcCache, 9);
	  encodeBuffer.encodeCachedValue(GetUINT(buffer + 8, bigEndian_), 16,
				   clientCache_.setClipRectanglesXCache, 8);
	  encodeBuffer.encodeCachedValue(GetUINT(buffer + 10, bigEndian_), 16,
				   clientCache_.setClipRectanglesYCache, 8);
	  const unsigned char *nextSrc = buffer + 12;
	  for (unsigned int i = 0; i < numRectangles; i++)
	  {
	    for (unsigned int j = 0; j < 4; j++)
	    {
	      encodeBuffer.encodeCachedValue(GetUINT(nextSrc, bigEndian_), 16,
			    *clientCache_.setClipRectanglesGeomCache[j], 8);
	      nextSrc += 2;
	    }
	  }
	}
	break;
      case X_SetDashes:
	{
	  unsigned int numDashes = GetUINT(buffer + 10, bigEndian_);
	  encodeBuffer.encodeCachedValue(numDashes, 16,
				      clientCache_.setDashesLengthCache, 5);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
					 clientCache_.gcCache, 9);
	  encodeBuffer.encodeCachedValue(GetUINT(buffer + 8, bigEndian_), 16,
				      clientCache_.setDashesOffsetCache, 5);
	  const unsigned char *nextSrc = buffer + 12;
	  for (unsigned int i = 0; i < numDashes; i++)
	    encodeBuffer.encodeCachedValue(*nextSrc++, 8,
				clientCache_.setDashesDashCache_[i & 1], 5);
	}
	break;
      case X_SetSelectionOwner:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
				    clientCache_.setSelectionOwnerCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8, bigEndian_), 29,
			   clientCache_.getSelectionOwnerSelectionCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 12, bigEndian_), 32,
			   clientCache_.setSelectionOwnerTimestampCache, 9);
	}
	break;
      case X_TranslateCoords:
	{
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 4, bigEndian_), 29,
				   clientCache_.translateCoordsSrcCache, 9);
	  encodeBuffer.encodeCachedValue(GetULONG(buffer + 8, bigEndian_), 29,
				  clientCache_.translateCoordsDestCache, 9);
	  encodeBuffer.encodeCachedValue(GetUINT(buffer + 12, bigEndian_), 16,
				     clientCache_.translateCoordsXCache, 8);
	  encodeBuffer.encodeCachedValue(GetUINT(buffer + 14, bigEndian_), 16,
				     clientCache_.translateCoordsYCache, 8);
	  sequenceNumQueue_.push(clientCache_.lastRequestSequenceNum, opcode);
	}
	break;
      default:
	{
	  encodeBuffer.encodeValue((unsigned int) buffer[1], 8);
	  encodeBuffer.encodeValue(GetUINT(buffer + 2, bigEndian_), 16, 8);
	  const unsigned char *nextSrc = buffer + 4;
	  for (unsigned int i = 4; i < size; i++)
	    encodeBuffer.encodeValue((unsigned int) *nextSrc++, 8);
	}
      }				// end switch

      stats_.add(*buffer, size << 3, encodeBuffer.getCumulativeBitsWritten());
    }				// end non-initial request

  }
  return 1;
}


int
  ClientChannel::
doWrite(const unsigned char *message, unsigned int length)
{
  writeBuffer_.reset();

  // uncompress messages
  DecodeBuffer decodeBuffer(message, length);
  if (firstReply_)
  {
    unsigned int opcode;
    decodeBuffer.decodeValue(opcode, 8);
    unsigned int secondByte;
    decodeBuffer.decodeValue(secondByte, 8);
    unsigned int major;
    decodeBuffer.decodeValue(major, 16);
    unsigned int minor;
    decodeBuffer.decodeValue(minor, 16);
    unsigned int extraLength;
    decodeBuffer.decodeValue(extraLength, 16);
    unsigned int outputLength = 8 + (extraLength << 2);
    unsigned char *outputMessage = writeBuffer_.addMessage(outputLength);
    *outputMessage = (unsigned char) opcode;
    outputMessage[1] = (unsigned char) secondByte;
    PutUINT(major, outputMessage + 2, bigEndian_);
    PutUINT(minor, outputMessage + 4, bigEndian_);
    PutUINT(extraLength, outputMessage + 6, bigEndian_);
    unsigned char *nextDest = outputMessage + 8;
    unsigned int cached;
    decodeBuffer.decodeValue(cached, 1);
    if (cached)
    memcpy(nextDest, ServerCache::lastInitReply.getData(), outputLength - 8);
    else
    {
      for (unsigned i = 8; i < outputLength; i++)
      {
	unsigned int nextByte;
	decodeBuffer.decodeValue(nextByte, 8);
	*nextDest++ = (unsigned char) nextByte;
      }
    ServerCache::lastInitReply.set(outputLength - 8, outputMessage + 8);
    }
    imageByteOrder_ = outputMessage[30];
    bitmapBitOrder_ = outputMessage[31];
    scanlineUnit_ = outputMessage[32];
    scanlinePad_ = outputMessage[32];
    firstReply_ = 0;
  }
  else
  {
    unsigned char opcode;
    while (decodeBuffer.decodeCachedValue(opcode, 8,
		   serverCache_.opcodeCache[serverCache_.lastOpcode], 8, 1))
    {
      serverCache_.lastOpcode = opcode;

      unsigned char *outputMessage = NULL;
      unsigned int outputLength = 0;
      unsigned int value;	// general-purpose temp variable for decoding ints

      unsigned char cValue;	// general-purpose temp variable for decoding chars

      if (opcode == 1)
      {
	// reply
	unsigned int sequenceNumDiff;
	decodeBuffer.decodeCachedValue(sequenceNumDiff, 16,
				     serverCache_.replySequenceNumCache, 7);
	unsigned int sequenceNum =
	serverCache_.lastSequenceNum + sequenceNumDiff;
	sequenceNum &= 0xffff;
	serverCache_.lastSequenceNum = sequenceNum;
	unsigned short int nextSequenceNum;
	unsigned char nextOpcode;
	if (sequenceNumQueue_.peek(nextSequenceNum, nextOpcode) &&
	    (nextSequenceNum == sequenceNum))
	{
	  unsigned int requestData[3];
	  sequenceNumQueue_.pop(nextSequenceNum, nextOpcode,
				requestData[0], requestData[1],
				requestData[2]);
	  switch (nextOpcode)
	  {
	  case X_AllocColor:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      unsigned char *nextDest = outputMessage + 8;
	      for (unsigned int i = 0; i < 3; i++)
	      {
		decodeBuffer.decodeValue(value, 1);
		if (value)
		  PutUINT(requestData[i], nextDest, bigEndian_);
		else
		{
		  decodeBuffer.decodeValue(value, 16, 6);
		  PutUINT(requestData[i] + value, nextDest, bigEndian_);
		}
		nextDest += 2;
	      }
	      decodeBuffer.decodeValue(value, 32, 9);
	      PutULONG(value, outputMessage + 16, bigEndian_);
	    }
	    break;
	  case X_GetAtomName:
	    {
	      unsigned int nameLength;
	      decodeBuffer.decodeValue(nameLength, 16, 6);
	      outputLength = RoundUp4(nameLength) + 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      PutUINT(nameLength, outputMessage + 8, bigEndian_);
	      unsigned char* nextDest = outputMessage + 32;
	      clientCache_.internAtomTextCompressor.reset();
	      for (unsigned int i = 0; i < nameLength; i++)
		*nextDest++ =
		  clientCache_.internAtomTextCompressor.decodeChar(
							 decodeBuffer);
	    }
	    break;
	  case X_GetGeometry:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeCachedValue(cValue, 8,
					     serverCache_.depthCache);
	      outputMessage[1] = cValue;
	      decodeBuffer.decodeCachedValue(value, 29,
				      serverCache_.getGeometryRootCache, 9);
	      PutULONG(value, outputMessage + 8, bigEndian_);
	      unsigned char *nextDest = outputMessage + 12;
	      for (unsigned int i = 0; i < 5; i++)
	      {
		decodeBuffer.decodeCachedValue(value, 16,
				  *serverCache_.getGeometryGeomCache[i], 8);
		PutUINT(value, nextDest, bigEndian_);
		nextDest += 2;
	      }
	    }
	    break;
	  case X_GetInputFocus:
	    {
	      outputLength = 32;
	      outputMessage =
		writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeValue(value, 2);
	      outputMessage[1] = (unsigned char) value;
	      decodeBuffer.decodeCachedValue(value, 29,
				  serverCache_.getInputFocusWindowCache, 9);
	      PutULONG(value, outputMessage + 8, bigEndian_);
	    }
	    break;
	  case X_GetKeyboardMapping:
	    {
	      decodeBuffer.decodeValue(value, 1);
	      if (value)
	      {
		unsigned int dataLength =
		ServerCache::getKeyboardMappingLastMap.getLength();
		outputLength = 32 + dataLength;
		outputMessage = writeBuffer_.addMessage(outputLength);
		outputMessage[1] =
	      ServerCache::getKeyboardMappingLastKeysymsPerKeycode;
		memcpy(outputMessage + 32,
	      ServerCache::getKeyboardMappingLastMap.getData(),
		       dataLength);
		break;
	      }
	      unsigned int numKeycodes;
	      decodeBuffer.decodeValue(numKeycodes, 8);
	      unsigned int keysymsPerKeycode;
	      decodeBuffer.decodeValue(keysymsPerKeycode, 8, 4);
	    ServerCache::getKeyboardMappingLastKeysymsPerKeycode =
		keysymsPerKeycode;
	      outputLength = 32 + numKeycodes * keysymsPerKeycode * 4;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      outputMessage[1] = (unsigned char) keysymsPerKeycode;
	      unsigned char *nextDest = outputMessage + 32;
	      unsigned char previous = 0;
	      for (unsigned int count = numKeycodes * keysymsPerKeycode;
		   count; --count)
	      {
		decodeBuffer.decodeValue(value, 1);
		if (value)
		  PutULONG((unsigned int) NoSymbol, nextDest, bigEndian_);
		else
		{
		  unsigned int keysym;
		  decodeBuffer.decodeCachedValue(keysym, 24,
			     serverCache_.getKeyboardMappingKeysymCache, 9);
		  decodeBuffer.decodeCachedValue(cValue, 8,
			   serverCache_.getKeyboardMappingLastByteCache, 5);
		  previous += cValue;
		  PutULONG((keysym << 8) | previous, nextDest, bigEndian_);
		}
		nextDest += 4;
	      }
	    ServerCache::getKeyboardMappingLastMap.set(outputLength - 32,
					    outputMessage + 32);
	    }
	    break;
	  case X_GetModifierMapping:
	    {
	      unsigned int keycodesPerModifier;
	      decodeBuffer.decodeValue(keycodesPerModifier, 8);
	      outputLength = 32 + (keycodesPerModifier << 3);
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      outputMessage[1] = (unsigned char) keycodesPerModifier;
	      unsigned char *nextDest = outputMessage + 32;
	      decodeBuffer.decodeValue(value, 1);
	      if (value)
	      {
		memcpy(outputMessage + 32,
	      ServerCache::getModifierMappingLastMap.getData(),
	      ServerCache::getModifierMappingLastMap.getLength());
		break;
	      }
	      for (unsigned int count = outputLength - 32; count; count--)
	      {
		decodeBuffer.decodeValue(value, 1);
		if (value)
		  *nextDest++ = 0;
		else
		{
		  decodeBuffer.decodeValue(value, 8);
		  *nextDest++ = value;
		}
	      }
	    ServerCache::getModifierMappingLastMap.set(outputLength - 32,
					    outputMessage + 32);
	    }
	    break;
	  case X_GetProperty:
	    {
	      unsigned char format;
	      decodeBuffer.decodeCachedValue(format, 8,
				       serverCache_.getPropertyFormatCache);
	      unsigned int length;
	      decodeBuffer.decodeValue(length, 32, 9);
	      unsigned int numBytes = length;
	      if (format == 16)
		numBytes <<= 1;
	      else if (format == 32)
		numBytes <<= 2;
	      outputLength = 32 + RoundUp4(numBytes);
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      outputMessage[1] = format;
	      PutULONG(length, outputMessage + 16, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 29,
				      serverCache_.getPropertyTypeCache, 9);
	      PutULONG(value, outputMessage + 8, bigEndian_);
	      decodeBuffer.decodeValue(value, 32, 9);
	      PutULONG(value, outputMessage + 12, bigEndian_);
	      unsigned char *nextDest = outputMessage + 32;
	      if (format == 8)
	      {
		if (requestData[0] == XA_RESOURCE_MANAGER)
		{
		  decodeBuffer.decodeValue(value, 1);
		  if (value)
		  {
		    memcpy(nextDest, ServerCache::xResources.getData(),
			   ServerCache::xResources.getLength());
		    break;
		  }
		}
		serverCache_.getPropertyTextCompressor.reset();
		for (unsigned int i = 0; i < numBytes; i++)
		{
		  unsigned char nextChar;
		  nextChar = *nextDest++ =
		    serverCache_.getPropertyTextCompressor.decodeChar(
							      decodeBuffer);
		  if (nextChar == 10)
		  {
		    serverCache_.getPropertyTextCompressor.reset(nextChar);
		  }
		}
		if (requestData[0] == XA_RESOURCE_MANAGER)
		  ServerCache::xResources.set(numBytes, outputMessage + 32);
	      }
	      else
	      {
		for (unsigned int i = 0; i < numBytes; i++)
		{
		  decodeBuffer.decodeValue(value, 8);
		  *nextDest++ = (unsigned char) value;
		}
	      }
	    }
	    break;
	  case X_GetSelectionOwner:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeCachedValue(value, 29,
				    serverCache_.getSelectionOwnerCache, 9);
	      PutULONG(value, outputMessage + 8, bigEndian_);
	    }
	    break;
	  case X_GetWindowAttributes:
	    {
	      outputLength = 44;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeValue(value, 2);
	      outputMessage[1] = (unsigned char) value;
	      decodeBuffer.decodeCachedValue(value, 29,
					     serverCache_.visualCache, 9);
	      PutULONG(value, outputMessage + 8, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
			     serverCache_.getWindowAttributesClassCache, 3);
	      PutUINT(value, outputMessage + 12, bigEndian_);
	      decodeBuffer.decodeCachedValue(cValue, 8,
			   serverCache_.getWindowAttributesBitGravityCache);
	      outputMessage[14] = cValue;
	      decodeBuffer.decodeCachedValue(cValue, 8,
			   serverCache_.getWindowAttributesWinGravityCache);
	      outputMessage[15] = cValue;
	      decodeBuffer.decodeCachedValue(value, 32,
			    serverCache_.getWindowAttributesPlanesCache, 9);
	      PutULONG(value, outputMessage + 16, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 32,
			     serverCache_.getWindowAttributesPixelCache, 9);
	      PutULONG(value, outputMessage + 20, bigEndian_);
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[24] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[25] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 2);
	      outputMessage[26] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[27] = (unsigned char) value;
	      decodeBuffer.decodeCachedValue(value, 29,
					     serverCache_.colormapCache, 9);
	      PutULONG(value, outputMessage + 28, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 32,
			    serverCache_.getWindowAttributesAllEventsCache);
	      PutULONG(value, outputMessage + 32, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 32,
			   serverCache_.getWindowAttributesYourEventsCache);
	      PutULONG(value, outputMessage + 36, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
			serverCache_.getWindowAttributesDontPropagateCache);
	      PutUINT(value, outputMessage + 40, bigEndian_);
	    }
	    break;
	  case X_GrabKeyboard:
	  case X_GrabPointer:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeValue(value, 3);
	      outputMessage[1] = (unsigned char) value;
	    }
	    break;
	  case X_InternAtom:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeValue(value, 29, 9);
	      PutULONG(value, outputMessage + 8, bigEndian_);
	    }
	    break;
	  case X_ListExtensions:
	    {
	      decodeBuffer.decodeValue(value, 32, 8);
	      outputLength = 32 + (value << 2);
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      unsigned int numExtensions;
	      decodeBuffer.decodeValue(numExtensions, 8);
	      outputMessage[1] = (unsigned char) numExtensions;
	      unsigned char *nextDest = outputMessage + 32;
	      for (; numExtensions; numExtensions--)
	      {
		unsigned int length;
		decodeBuffer.decodeValue(length, 8);
		*nextDest++ = (unsigned char) length;
		for (; length; length--)
		{
		  decodeBuffer.decodeValue(value, 8);
		  *nextDest++ = value;
		}
	      }
	    }
	    break;
	  case X_ListFonts:
	    {
	      decodeBuffer.decodeValue(value, 32, 8);
	      outputLength = 32 + (value << 2);
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      unsigned int numFonts;
	      decodeBuffer.decodeValue(numFonts, 16, 6);
	      PutUINT(numFonts, outputMessage + 8, bigEndian_);
	      unsigned char* nextDest = outputMessage + 32;
	      for (; numFonts; numFonts--)
	      {
		unsigned int length;
		decodeBuffer.decodeValue(length, 8);
		*nextDest++ = (unsigned char)length;
		serverCache_.getPropertyTextCompressor.reset();
		for (; length; length--)
		  *nextDest++ =
		    serverCache_.getPropertyTextCompressor.decodeChar(
							      decodeBuffer);
	      }
	    }
	    break;
	  case X_LookupColor:
	  case X_AllocNamedColor:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      unsigned char *nextDest = outputMessage + 8;
	      if (nextOpcode == X_AllocNamedColor)
	      {
		decodeBuffer.decodeValue(value, 32, 9);
		PutULONG(value, nextDest, bigEndian_);
		nextDest += 4;
	      }
	      unsigned int count = 3;
	      do
	      {
		decodeBuffer.decodeValue(value, 16, 9);
		PutUINT(value, nextDest, bigEndian_);
		unsigned int visualColor;
		decodeBuffer.decodeValue(visualColor, 16, 5);
		visualColor += value;
		visualColor &= 0xffff;
		PutUINT(visualColor, nextDest + 6, bigEndian_);
		nextDest += 2;
	      }
	      while (--count);
	    }
	    break;
	  case X_QueryBestSize:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeValue(value, 16, 8);
	      PutUINT(value, outputMessage + 8, bigEndian_);
	      decodeBuffer.decodeValue(value, 16, 8);
	      PutUINT(value, outputMessage + 10, bigEndian_);
	    }
	    break;
	  case X_QueryColors:
	    {
	      unsigned int cached;
	      decodeBuffer.decodeValue(cached, 1, 1);
	      if (cached)
	      {
		unsigned int numColors =
		serverCache_.queryColorsLastReply.getLength() / 6;
		outputLength = 32 + (numColors << 3);
		outputMessage = writeBuffer_.addMessage(outputLength);
		PutUINT(numColors, outputMessage + 8, bigEndian_);
		const unsigned char *nextSrc =
		serverCache_.queryColorsLastReply.getData();
		unsigned char *nextDest = outputMessage + 32;
		for (; numColors; numColors--)
		{
		  for (unsigned int i = 0; i < 6; i++)
		    *nextDest++ = *nextSrc++;
		  nextDest += 2;
		}
	      }
	      else
	      {
		unsigned int numColors;
		decodeBuffer.decodeValue(numColors, 16, 5);
		outputLength = 32 + (numColors << 3);
		outputMessage = writeBuffer_.addMessage(outputLength);
		PutUINT(numColors, outputMessage + 8, bigEndian_);
		unsigned char *nextDest = outputMessage + 32;
		for (unsigned int c = 0; c < numColors; c++)
		{
		  for (unsigned int i = 0; i < 3; i++)
		  {
		    decodeBuffer.decodeValue(value, 16);
		    PutUINT(value, nextDest, bigEndian_);
		    nextDest += 2;
		  }
		}
		serverCache_.queryColorsLastReply.set(numColors * 6,
						      outputMessage + 32);
		const unsigned char *nextSrc = nextDest - 1;
		nextDest = outputMessage + 32 + ((numColors - 1) << 3) + 5;
		for (; numColors > 1; numColors--)
		{
		  for (unsigned int i = 0; i < 6; i++)
		    *nextDest-- = *nextSrc--;
		  nextDest -= 2;
		}
	      }
	    }
	    break;
	  case X_QueryExtension:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[8] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 8);
	      outputMessage[9] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 8);
	      outputMessage[10] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 8);
	      outputMessage[11] = (unsigned char) value;
	    }
	    break;
	  case X_QueryFont:
	    {
	      unsigned int numProperties;
	      unsigned int numCharInfos;
	      decodeBuffer.decodeValue(numProperties, 16, 8);
	      decodeBuffer.decodeValue(numCharInfos, 32, 10);
	      outputLength = 60 + numProperties * 8 + numCharInfos * 12;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      PutUINT(numProperties, outputMessage + 46, bigEndian_);
	      PutULONG(numCharInfos, outputMessage + 56, bigEndian_);
	      decodeCharInfo_(decodeBuffer, outputMessage + 8);
	      decodeCharInfo_(decodeBuffer, outputMessage + 24);
	      decodeBuffer.decodeValue(value, 16, 9);
	      PutUINT(value, outputMessage + 40, bigEndian_);
	      decodeBuffer.decodeValue(value, 16, 9);
	      PutUINT(value, outputMessage + 42, bigEndian_);
	      decodeBuffer.decodeValue(value, 16, 9);
	      PutUINT(value, outputMessage + 44, bigEndian_);
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[48] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 8);
	      outputMessage[49] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 8);
	      outputMessage[50] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[51] = (unsigned char) value;
	      decodeBuffer.decodeValue(value, 16, 9);
	      PutUINT(value, outputMessage + 52, bigEndian_);
	      decodeBuffer.decodeValue(value, 16, 9);
	      PutUINT(value, outputMessage + 54, bigEndian_);
	      unsigned char *nextDest = outputMessage + 60;
	      decodeBuffer.decodeValue(value, 1);
	      if (value)
	      {
		unsigned int index;
		decodeBuffer.decodeValue(index, 4);
		unsigned int length;
		const unsigned char *data;
	      ServerCache::queryFontFontCache.get(index, length, data);
		memcpy(nextDest, data, length);
		break;
	      }
	      unsigned char *saveDest = nextDest;
	      unsigned int length = numProperties * 8 + numCharInfos * 12;
	      for (; numProperties; numProperties--)
	      {
		decodeBuffer.decodeValue(value, 32, 9);
		PutULONG(value, nextDest, bigEndian_);
		decodeBuffer.decodeValue(value, 32, 9);
		PutULONG(value, nextDest + 4, bigEndian_);
		nextDest += 8;
	      }
	      for (; numCharInfos; numCharInfos--)
	      {
		decodeCharInfo_(decodeBuffer, nextDest);
		nextDest += 12;
	      }
	    ServerCache::queryFontFontCache.set(length, saveDest);
	    }
	    break;
	  case X_QueryPointer:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[1] = (unsigned char) value;
	      decodeBuffer.decodeCachedValue(value, 29,
				     serverCache_.queryPointerRootCache, 9);
	      PutULONG(value, outputMessage + 8, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 29,
				    serverCache_.queryPointerChildCache, 9);
	      PutULONG(value, outputMessage + 12, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				    serverCache_.motionNotifyRootXCache, 8);
	      serverCache_.motionNotifyLastRootX += value;
	      PutUINT(serverCache_.motionNotifyLastRootX, outputMessage + 16,
		      bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				    serverCache_.motionNotifyRootYCache, 8);
	      serverCache_.motionNotifyLastRootY += value;
	      PutUINT(serverCache_.motionNotifyLastRootY, outputMessage + 18,
		      bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				   serverCache_.motionNotifyEventXCache, 8);
	      PutUINT(serverCache_.motionNotifyLastRootX + value,
		      outputMessage + 20, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				   serverCache_.motionNotifyEventYCache, 8);
	      PutUINT(serverCache_.motionNotifyLastRootY + value,
		      outputMessage + 22, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				       serverCache_.motionNotifyStateCache);
	      PutUINT(value, outputMessage + 24, bigEndian_);
	    }
	    break;
	  case X_QueryTree:
	    {
	      unsigned int secondByte;
	      decodeBuffer.decodeValue(secondByte, 8);
	      unsigned int replyLength;
	      decodeBuffer.decodeValue(replyLength, 32);
	      outputLength = 32 + (replyLength << 2);
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      outputMessage[1] = (unsigned char) secondByte;
	      unsigned char *nextDest = outputMessage + 8;
	      for (unsigned int i = 8; i < outputLength; i++)
	      {
		unsigned int nextByte;
		decodeBuffer.decodeValue(nextByte, 8);
		*nextDest++ = (unsigned char) nextByte;
	      }
	    }
	    break;
	  case X_TranslateCoords:
	    {
	      outputLength = 32;
	      outputMessage = writeBuffer_.addMessage(outputLength);
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[1] = (unsigned char) value;
	      decodeBuffer.decodeCachedValue(value, 29,
				 serverCache_.translateCoordsChildCache, 9);
	      PutULONG(value, outputMessage + 8, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				     serverCache_.translateCoordsXCache, 8);
	      PutUINT(value, outputMessage + 12, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				     serverCache_.translateCoordsYCache, 8);
	      PutUINT(value, outputMessage + 14, bigEndian_);
	    }
	    break;
	  default:
	    {
	      cerr <<
		"assertion failed in ClientProxyReader::processMessage():\n" <<
		" no matching request for reply with sequence number " <<
		sequenceNum << endl;
	    }
	  }
	}
	else
	{
	  unsigned int secondByte;
	  decodeBuffer.decodeValue(secondByte, 8);
	  unsigned int replyLength;
	  decodeBuffer.decodeValue(replyLength, 32);
	  outputLength = 32 + (replyLength << 2);
	  outputMessage = writeBuffer_.addMessage(outputLength);
	  outputMessage[1] = (unsigned char) secondByte;
	  unsigned char *nextDest = outputMessage + 8;
	  for (unsigned int i = 8; i < outputLength; i++)
	  {
	    unsigned int nextByte;
	    decodeBuffer.decodeValue(nextByte, 8);
	    *nextDest++ = (unsigned char) nextByte;
	  }
	}
	PutULONG((outputLength - 32) >> 2, outputMessage + 4, bigEndian_);
      }
      else
      {
	// event or error
	unsigned int sequenceNumDiff;
	decodeBuffer.decodeCachedValue(sequenceNumDiff, 16,
				     serverCache_.eventSequenceNumCache, 7);
	serverCache_.lastSequenceNum += sequenceNumDiff;
	serverCache_.lastSequenceNum &= 0xffff;

	outputLength = 32;
	outputMessage = writeBuffer_.addMessage(outputLength);

	// check if this is an error that matches a sequence number for
	// which we were expecting a reply
	unsigned short int dummySequenceNum;
	unsigned char dummyOpcode;
	if (sequenceNumQueue_.peek(dummySequenceNum, dummyOpcode) &&
	  ((unsigned int) dummySequenceNum == serverCache_.lastSequenceNum))
	  sequenceNumQueue_.pop(dummySequenceNum, dummyOpcode);

	switch (opcode)
	{
	case 0:
	  {
	    unsigned char code;
	    decodeBuffer.decodeCachedValue(code, 8,
					   serverCache_.errorCodeCache);
	    outputMessage[1] = code;
	    if ((code != 11) && (code != 8) && (code != 15) &&
		(code != 1))
	    {
	      decodeBuffer.decodeValue(value, 32, 16);
	      PutULONG(value, outputMessage + 4, bigEndian_);
	    }
	    if (code >= 18)
	    {
	      decodeBuffer.decodeCachedValue(value, 16,
					     serverCache_.errorMinorCache);
	      PutUINT(value, outputMessage + 8, bigEndian_);
	    }
	    decodeBuffer.decodeCachedValue(cValue, 8,
					   serverCache_.errorMajorCache);
	    outputMessage[10] = cValue;
	    if (code >= 18)
	    {
	      unsigned char *nextDest = outputMessage + 11;
	      for (unsigned int i = 11; i < 32; i++)
	      {
		decodeBuffer.decodeValue(value, 8);
		*nextDest++ = (unsigned char) cValue;
	      }
	    }
	  }
	  break;
	case ButtonPress:
	case ButtonRelease:
	case KeyPress:
	case KeyRelease:
	case MotionNotify:
	case EnterNotify:
	case LeaveNotify:
	  {
	    if (opcode == MotionNotify)
	      decodeBuffer.decodeValue(value, 1);
	    else if ((opcode == EnterNotify) || (opcode == LeaveNotify))
	      decodeBuffer.decodeValue(value, 3);
	    else if (opcode == KeyRelease)
	    {
	      decodeBuffer.decodeValue(value, 1);
	      if (value)
		value = serverCache_.keyPressLastKey;
	      else
		decodeBuffer.decodeValue(value, 8);
	    }
	    else if ((opcode == ButtonPress) || (opcode == ButtonRelease))
	    {
	      decodeBuffer.decodeCachedValue(cValue, 8,
					     serverCache_.buttonCache);
	      value = (unsigned int) cValue;
	    }
	    else
	      decodeBuffer.decodeValue(value, 8);
	    outputMessage[1] = (unsigned char) value;
	    decodeBuffer.decodeCachedValue(value, 32,
				serverCache_.motionNotifyTimestampCache, 9);
	    serverCache_.lastTimestamp += value;
	    PutULONG(serverCache_.lastTimestamp, outputMessage + 4,
		     bigEndian_);
	    unsigned char *nextDest = outputMessage + 8;
	    int skipRest = 0;
	    if (opcode == KeyRelease)
	    {
	      decodeBuffer.decodeValue(value, 1);
	      if (value)
	      {
		for (unsigned int i = 0; i < 23; i++)
		  *nextDest++ = serverCache_.keyPressCache[i];
		skipRest = 1;
	      }
	    }
	    if (!skipRest)
	    {
	      for (unsigned int i = 0; i < 3; i++)
	      {
		decodeBuffer.decodeCachedValue(value, 29,
			       *serverCache_.motionNotifyWindowCache[i], 6);
		PutULONG(value, nextDest, bigEndian_);
		nextDest += 4;
	      }
	      decodeBuffer.decodeCachedValue(value, 16,
				    serverCache_.motionNotifyRootXCache, 6);
	      serverCache_.motionNotifyLastRootX += value;
	      PutUINT(serverCache_.motionNotifyLastRootX, outputMessage + 20,
		      bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				    serverCache_.motionNotifyRootYCache, 6);
	      serverCache_.motionNotifyLastRootY += value;
	      PutUINT(serverCache_.motionNotifyLastRootY, outputMessage + 22,
		      bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				   serverCache_.motionNotifyEventXCache, 6);
	      PutUINT(serverCache_.motionNotifyLastRootX + value,
		      outputMessage + 24, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				   serverCache_.motionNotifyEventYCache, 6);
	      PutUINT(serverCache_.motionNotifyLastRootY + value,
		      outputMessage + 26, bigEndian_);
	      decodeBuffer.decodeCachedValue(value, 16,
				       serverCache_.motionNotifyStateCache);
	      PutUINT(value, outputMessage + 28, bigEndian_);
	      if ((opcode == EnterNotify) || (opcode == LeaveNotify))
		decodeBuffer.decodeValue(value, 2);
	      else
		decodeBuffer.decodeValue(value, 1);
	      outputMessage[30] = (unsigned char) value;
	      if ((opcode == EnterNotify) || (opcode == LeaveNotify))
	      {
		decodeBuffer.decodeValue(value, 2);
		outputMessage[31] = (unsigned char) value;
	      }
	      else if (opcode == KeyPress)
	      {
		serverCache_.keyPressLastKey = outputMessage[1];
		for (unsigned int i = 8; i < 31; i++)
		{
		  serverCache_.keyPressCache[i - 8] = outputMessage[i];
		}
	      }
	    }
	  }
	  break;
	case ColormapNotify:
	  {
	    decodeBuffer.decodeCachedValue(value, 29,
				 serverCache_.colormapNotifyWindowCache, 8);
	    PutULONG(value, outputMessage + 4, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
			       serverCache_.colormapNotifyColormapCache, 8);
	    PutULONG(value, outputMessage + 8, bigEndian_);
	    decodeBuffer.decodeValue(value, 1);
	    outputMessage[12] = (unsigned char) value;
	    decodeBuffer.decodeValue(value, 1);
	    outputMessage[13] = (unsigned char) value;
	  }
	  break;
	case ConfigureNotify:
	  {
	    unsigned char *nextDest = outputMessage + 4;
	    for (unsigned int i = 0; i < 3; i++)
	    {
	      decodeBuffer.decodeCachedValue(value, 29,
			    *serverCache_.configureNotifyWindowCache[i], 9);
	      PutULONG(value, nextDest, bigEndian_);
	      nextDest += 4;
	    }
	    for (unsigned int j = 0; j < 5; j++)
	    {
	      decodeBuffer.decodeCachedValue(value, 16,
			      *serverCache_.configureNotifyGeomCache[j], 8);
	      PutUINT(value, nextDest, bigEndian_);
	      nextDest += 2;
	    }
	    decodeBuffer.decodeValue(value, 1);
	    *nextDest = value;
	  }
	  break;
	case CreateNotify:
	  {
	    decodeBuffer.decodeCachedValue(value, 29,
				 serverCache_.createNotifyWindowCache, 9);
	    PutULONG(value, outputMessage + 4, bigEndian_);
	    decodeBuffer.decodeValue(value, 29, 5);
	    serverCache_.createNotifyLastWindow += value;
	    serverCache_.createNotifyLastWindow &= 0x1fffffff;
	    PutULONG(serverCache_.createNotifyLastWindow, outputMessage + 8,
		     bigEndian_);
	    unsigned char* nextDest = outputMessage + 12;
	    for (unsigned int i = 0; i < 5; i++)
	    {
	      decodeBuffer.decodeValue(value, 16, 9);
	      PutUINT(value, nextDest, bigEndian_);
	      nextDest += 2;
	    }
	    decodeBuffer.decodeValue(value, 1);
	    *nextDest = (unsigned char)value;
	  }
	  break;
	case Expose:
	  {
	    decodeBuffer.decodeCachedValue(value, 29,
					 serverCache_.exposeWindowCache, 9);
	    PutULONG(value, outputMessage + 4, bigEndian_);
	    unsigned char *nextDest = outputMessage + 8;
	    for (unsigned int i = 0; i < 5; i++)
	    {
	      decodeBuffer.decodeCachedValue(value, 16,
				       *serverCache_.exposeGeomCache[i], 6);
	      PutUINT(value, nextDest, bigEndian_);
	      nextDest += 2;
	    }
	  }
	  break;
	case FocusIn:
	case FocusOut:
	  {
	    decodeBuffer.decodeValue(value, 3);
	    outputMessage[1] = (unsigned char) value;
	    decodeBuffer.decodeCachedValue(value, 29,
					serverCache_.focusInWindowCache, 9);
	    PutULONG(value, outputMessage + 4, bigEndian_);
	    decodeBuffer.decodeValue(value, 2);
	    outputMessage[8] = (unsigned char) value;
	  }
	  break;
	case KeymapNotify:
	  {
	    decodeBuffer.decodeValue(value, 1);
	    if (value)
	    memcpy(outputMessage + 1, ServerCache::lastKeymap.getData(), 31);
	    else
	    {
	      unsigned char *nextDest = outputMessage + 1;
	      for (unsigned int i = 1; i < 32; i++)
	      {
		decodeBuffer.decodeValue(value, 8);
		*nextDest++ = (unsigned char) value;
	      }
	    ServerCache::lastKeymap.set(31, outputMessage + 1);
	    }
	  }
	  break;
	case MapNotify:
	case UnmapNotify:
	case DestroyNotify:
	  {
	    decodeBuffer.decodeCachedValue(value, 29,
				       serverCache_.mapNotifyEventCache, 9);
	    PutULONG(value, outputMessage + 4, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				      serverCache_.mapNotifyWindowCache, 9);
	    PutULONG(value, outputMessage + 8, bigEndian_);
	    if ((opcode == MapNotify) || (opcode == UnmapNotify))
	    {
	      decodeBuffer.decodeValue(value, 1);
	      outputMessage[12] = (unsigned char) value;
	    }
	  }
	  break;
	case NoExpose:
	  {
	    decodeBuffer.decodeCachedValue(value, 29,
				     serverCache_.noExposeDrawableCache, 9);
	    PutULONG(value, outputMessage + 4, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 16,
					   serverCache_.noExposeMinorCache);
	    PutUINT(value, outputMessage + 8, bigEndian_);
	    decodeBuffer.decodeCachedValue(cValue, 8,
					   serverCache_.noExposeMajorCache);
	    outputMessage[10] = cValue;
	  }
	  break;
	case PropertyNotify:
	  {
	    decodeBuffer.decodeCachedValue(value, 29,
				 serverCache_.propertyNotifyWindowCache, 9);
	    PutULONG(value, outputMessage + 4, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				   serverCache_.propertyNotifyAtomCache, 9);
	    PutULONG(value, outputMessage + 8, bigEndian_);
	    decodeBuffer.decodeValue(value, 32, 9);
	    serverCache_.lastTimestamp += value;
	    PutULONG(serverCache_.lastTimestamp, outputMessage + 12,
		     bigEndian_);
	    decodeBuffer.decodeValue(value, 1);
	    outputMessage[16] = (unsigned char) value;
	  }
	  break;
	case ReparentNotify:
	  {
	    unsigned char* nextDest = outputMessage + 4;
	    for (unsigned int i = 0; i < 3; i++)
	    {
	      decodeBuffer.decodeCachedValue(value, 29,
			     serverCache_.reparentNotifyWindowCache, 9);
	      PutULONG(value, nextDest, bigEndian_);
	      nextDest += 4;
	    }
	    decodeBuffer.decodeValue(value, 16, 6);
	    PutUINT(value, nextDest, bigEndian_);
	    decodeBuffer.decodeValue(value, 16, 6);
	    PutUINT(value, nextDest + 2, bigEndian_);
	    decodeBuffer.decodeValue(value, 1);
	    outputMessage[20] = (unsigned char)value;
	  }
	  break;
	case SelectionClear:
	  {
	    decodeBuffer.decodeValue(value, 32, 9);
	    serverCache_.lastTimestamp += value;
	    PutULONG(serverCache_.lastTimestamp, outputMessage + 4,
		     bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				   serverCache_.selectionClearWindowCache, 9);
	    PutULONG(value, outputMessage + 8, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				   serverCache_.selectionClearAtomCache, 9);
	    PutULONG(value, outputMessage + 12, bigEndian_);
	  }
	  break;
	case SelectionRequest:
	  {
	    decodeBuffer.decodeValue(value, 32, 9);
	    serverCache_.lastTimestamp += value;
	    PutULONG(serverCache_.lastTimestamp, outputMessage + 4,
		     bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				   serverCache_.selectionClearWindowCache, 9);
	    PutULONG(value, outputMessage + 8, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				   serverCache_.selectionClearWindowCache, 9);
	    PutULONG(value, outputMessage + 12, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				   serverCache_.selectionClearAtomCache, 9);
	    PutULONG(value, outputMessage + 16, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				   serverCache_.selectionClearAtomCache, 9);
	    PutULONG(value, outputMessage + 20, bigEndian_);
	    decodeBuffer.decodeCachedValue(value, 29,
				   serverCache_.selectionClearAtomCache, 9);
	    PutULONG(value, outputMessage + 24, bigEndian_);
	  }
	  break;
	case VisibilityNotify:
	  {
	    decodeBuffer.decodeCachedValue(value, 29,
			       serverCache_.visibilityNotifyWindowCache, 9);
	    PutULONG(value, outputMessage + 4, bigEndian_);
	    decodeBuffer.decodeValue(value, 2);
	    outputMessage[8] = (unsigned char) value;
	  }
	  break;
	default:
	  {
	    unsigned int secondByte;
	    decodeBuffer.decodeValue(secondByte, 8);
	    outputMessage[1] = secondByte;
	    unsigned char *nextDest = outputMessage + 4;
	    for (unsigned int i = 4; i < outputLength; i++)
	    {
	      unsigned int nextByte;
	      decodeBuffer.decodeValue(nextByte, 8);
	      *nextDest++ = (unsigned char) nextByte;
	    }
	  }
	}
      }
      *outputMessage = (unsigned char) opcode;
      PutUINT(serverCache_.lastSequenceNum, outputMessage + 2, bigEndian_);
    }
  }

  if (WriteAll(fd_, writeBuffer_.getData(), writeBuffer_.getLength()) < 0)
    return 0;
  else
    return 1;
}


void
ClientChannel::setBigEndian(int flag)
{
  bigEndian_ = flag;
}



void
ClientChannel::decodeCharInfo_(DecodeBuffer& decodeBuffer,
			       unsigned char*nextDest)
{
  unsigned int value;
  decodeBuffer.decodeCachedValue(value, 32,
				 *serverCache_.queryFontCharInfoCache[0], 6);
  PutUINT(value & 0xffff, nextDest, bigEndian_);
  PutUINT(value >> 16, nextDest + 10, bigEndian_);
  nextDest += 2;
  for (unsigned int i = 1; i < 5; i++)
  {
    unsigned int value;
    decodeBuffer.decodeCachedValue(value, 16,
				*serverCache_.queryFontCharInfoCache[i], 6);
    PutUINT(value, nextDest, bigEndian_);
    nextDest += 2;
  }
}
