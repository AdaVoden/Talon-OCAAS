#ifndef Stats_H
#define Stats_H

class Stats
{
  public:
  Stats();
  ~Stats();

  void add(unsigned int opcode, unsigned int bitsIn, unsigned int bitsOut);
  void summarize(unsigned int &bitsIn, unsigned int &bitsOut,
		 int showDetails = 0);

    private:
  unsigned int count_[257];
  unsigned int bitsIn_[257];
  unsigned int bitsOut_[257];
};

#endif /* Stats_H */
