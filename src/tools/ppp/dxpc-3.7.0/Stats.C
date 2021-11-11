#include <iostream.h>
#include "Stats.H"
#include "util.H"

Stats::Stats()
{
  for (unsigned int i = 0; i < 257; i++)
  {
    count_[i] = 0;
    bitsIn_[i] = 0;
    bitsOut_[i] = 0;
  }
}


Stats::~Stats()
{
}


void
Stats::add(unsigned int opcode, unsigned int bitsIn, unsigned int bitsOut)
{
  count_[opcode]++;
  bitsIn_[opcode] += bitsIn;
  bitsOut_[opcode] += bitsOut;
}


void
Stats::summarize(unsigned int &bitsIn, unsigned int &bitsOut, int showDetails)
{
  unsigned int totalBitsIn = 0;
  unsigned int totalBitsOut = 0;

  if (showDetails)
  {
    *logofs << "\nmsg\t\tbits\tbits\tcompression" << endl;
    *logofs << "type\tcount\tin\tout\tratio" << endl;
    *logofs << "----\t-----\t-----\t-----\t-----------" << endl;
  }

  for (unsigned int i = 0; i < 257; i++)
    if (count_[i])
    {
      totalBitsIn += bitsIn_[i];
      totalBitsOut += bitsOut_[i];
      if (showDetails)
      {
	if (i == 256)
	{
	  *logofs << "other";
	}
	else
	{
	  *logofs << i;
	}
	*logofs << '\t' << count_[i] << '\t' <<
	  bitsIn_[i] << '\t' << bitsOut_[i] << '\t' <<
	  (float) bitsIn_[i] / (float) bitsOut_[i] << ":1" <<
	  endl;
      }
    }

  bitsIn = totalBitsIn;
  bitsOut = totalBitsOut;
}
