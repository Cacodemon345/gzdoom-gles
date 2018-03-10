#include "critsec.h"
#include "musicblock.h"

class FileRdr;

class OPLmusicBlock : public musicBlock
{
public:
	OPLmusicBlock();
	virtual ~OPLmusicBlock();

	bool ServiceStream(void *buff, int numbytes);
	void ResetChips();

	virtual void Restart();

protected:
	virtual int PlayTick() = 0;
	void OffsetSamples(float *buff, int count);

	uint8_t *score;
	uint8_t *scoredata;
	int playingcount;
	double NextTickIn;
	double SamplesPerTick;
	int NumChips;
	bool Looping;
	double LastOffset;
	bool FullPan;

	FCriticalSection ChipAccess;
};

class OPLmusicFile : public OPLmusicBlock
{
public:
	OPLmusicFile(FileRdr &reader);
	OPLmusicFile(const OPLmusicFile *source, const char *filename);
	virtual ~OPLmusicFile();

	bool IsValid() const;
	void SetLooping(bool loop);
	void Restart();
	void Dump();

protected:
	OPLmusicFile() {}
	int PlayTick();

	enum { RDosPlay, IMF, DosBox1, DosBox2 } RawPlayer;
	int ScoreLen;
	int WhichChip;
};
