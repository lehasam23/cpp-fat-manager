#ifndef FATPARSER_H
#define FATPARSER_H

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QList>
#include <QString>

struct FatDirEntry
{
	QString name;
	bool isDirectory;
	bool isDeleted;
	uint32_t startCluster;
	uint32_t size;
	QDateTime modificationTime;
};

class FatParser
{
  public:
	FatParser(const QString &filePath);
	~FatParser();

	bool isValid() const;
	uint32_t getRootCluster() const;
	QByteArray readCluster(uint32_t cluster);
	QList< uint32_t > getClusterChain(uint32_t startCluster);
	QList< FatDirEntry > readDirectory(uint32_t startCluster);
	QByteArray readFile(uint32_t startCluster, uint32_t size);

  private:
	struct BootSector
	{
		uint16_t bytesPerSector;
		uint8_t sectorsPerCluster;
		uint16_t reservedSectors;
		uint8_t fatCount;
		uint32_t sectorsPerFat;
		uint32_t rootCluster;
		uint32_t rootDirSectors;
	};

	bool parseBootSector();
	QDateTime readTimeDirectly(const QByteArray &entryData, int offset);
	QFile file;
	bool valid;
	BootSector bootSector;
	uint32_t dataOffset;
};

#endif	  // FATPARSER_H
