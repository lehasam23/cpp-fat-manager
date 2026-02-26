#include "include/fatparser.h"

#include <QDataStream>
#include <QMessageBox>

FatParser::FatParser(const QString &filePath) : valid(false), dataOffset(0)
{
	file.setFileName(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::critical(nullptr, "Error", "Cannot open file");
		return;
	}
	valid = parseBootSector();
}

FatParser::~FatParser()
{
	file.close();
}

bool FatParser::isValid() const
{
	return valid;
}

uint32_t FatParser::getRootCluster() const
{
	return valid ? bootSector.rootCluster : 0;
}

bool FatParser::parseBootSector()
{
	if (!file.seek(0))
		return false;
	QByteArray bootData = file.read(512);
	if (bootData.size() != 512 || (uint8_t)bootData[510] != 0x55 || (uint8_t)bootData[511] != 0xAA)
	{
		return false;
	}

	QDataStream stream(bootData);
	stream.setByteOrder(QDataStream::LittleEndian);

	stream.device()->seek(11);
	stream >> bootSector.bytesPerSector;
	stream >> bootSector.sectorsPerCluster;
	stream >> bootSector.reservedSectors;
	stream >> bootSector.fatCount;

	uint16_t rootDirEntries;
	stream.device()->seek(17);
	stream >> rootDirEntries;

	bool isFAT32 = (rootDirEntries == 0);
	if (isFAT32)
	{
		stream.device()->seek(36);
		stream >> bootSector.sectorsPerFat;
		stream.device()->seek(44);
		stream >> bootSector.rootCluster;
		bootSector.rootDirSectors = 0;
	}
	else
	{
		stream.device()->seek(22);
		uint16_t sectorsPerFat;
		stream >> sectorsPerFat;
		bootSector.sectorsPerFat = sectorsPerFat;
		bootSector.rootCluster = 0;
		bootSector.rootDirSectors = (rootDirEntries * 32 + bootSector.bytesPerSector - 1) / bootSector.bytesPerSector;
	}

	uint32_t totalSectors;
	stream.device()->seek(19);
	stream >> totalSectors;
	if (totalSectors == 0)
	{
		stream.device()->seek(32);
		stream >> totalSectors;
	}

	uint32_t dataSectors =
		totalSectors - (bootSector.reservedSectors + bootSector.fatCount * bootSector.sectorsPerFat + bootSector.rootDirSectors);
	uint32_t clusterCount = dataSectors / bootSector.sectorsPerCluster;
	if (clusterCount < 4085)
		return false;

	dataOffset = (bootSector.reservedSectors + bootSector.fatCount * bootSector.sectorsPerFat + bootSector.rootDirSectors) *
				 bootSector.bytesPerSector;
	return true;
}

QByteArray FatParser::readCluster(uint32_t cluster)
{
	if (!valid || cluster < 2)
		return QByteArray();
	uint32_t sector = (cluster - 2) * bootSector.sectorsPerCluster + dataOffset / bootSector.bytesPerSector;
	if (!file.seek(static_cast< qint64 >(sector) * bootSector.bytesPerSector))
		return QByteArray();
	return file.read(static_cast< qint64 >(bootSector.sectorsPerCluster) * bootSector.bytesPerSector);
}

QList< uint32_t > FatParser::getClusterChain(uint32_t startCluster)
{
	QList< uint32_t > chain;
	if (!valid || startCluster < 2)
		return chain;

	uint32_t currentCluster = startCluster;
	uint32_t fatOffset = bootSector.reservedSectors * bootSector.bytesPerSector;
	int fatEntrySize = bootSector.rootCluster ? 4 : 2;

	QDataStream stream(&file);
	stream.setByteOrder(QDataStream::LittleEndian);

	while (currentCluster >= 2)
	{
		chain.append(currentCluster);
		if (!file.seek(fatOffset + static_cast< qint64 >(currentCluster) * fatEntrySize))
			break;
		uint32_t nextCluster;
		if (fatEntrySize == 4)
		{
			stream >> nextCluster;
			nextCluster &= 0x0FFFFFFF;
			if (nextCluster >= 0x0FFFFFF8)
				break;
		}
		else
		{
			uint16_t next;
			stream >> next;
			nextCluster = next;
			if (nextCluster >= 0xFFF8)
				break;
		}
		currentCluster = nextCluster;
	}
	return chain;
}

QDateTime FatParser::readTimeDirectly(const QByteArray &entryData, int offset)
{
	if (entryData.size() < offset + 4)
		return QDateTime();

	uint16_t time = (uint8_t)entryData[offset] | ((uint8_t)entryData[offset + 1] << 8);
	uint16_t date = (uint8_t)entryData[offset + 2] | ((uint8_t)entryData[offset + 3] << 8);

	if (!time || !date)
		return QDateTime();

	int year = ((date >> 9) & 0x7F) + 1980;
	int month = (date >> 5) & 0x0F;
	int day = date & 0x1F;

	int hours = (time >> 11) & 0x1F;
	int minutes = (time >> 5) & 0x3F;
	int seconds = (time & 0x1F) * 2;

	return QDateTime(QDate(year, month, day), QTime(hours, minutes, seconds));
}

QList< FatDirEntry > FatParser::readDirectory(uint32_t startCluster)
{
	QList< FatDirEntry > entries;
	QList< QByteArray > lfnEntries;
	QByteArray dirData;

	if (!valid)
		return entries;

	if (!bootSector.rootCluster && startCluster == 0)
	{
		qint64 offset =
			static_cast< qint64 >(bootSector.reservedSectors + bootSector.fatCount * bootSector.sectorsPerFat) *
			bootSector.bytesPerSector;
		if (!file.seek(offset))
			return entries;
		dirData = file.read(static_cast< qint64 >(bootSector.rootDirSectors) * bootSector.bytesPerSector);
	}
	else
	{
		for (uint32_t cluster : getClusterChain(startCluster))
		{
			QByteArray clusterData = readCluster(cluster);
			if (clusterData.isEmpty())
				break;
			dirData.append(clusterData);
		}
	}

	for (int i = 0; i < dirData.size(); i += 32)
	{
		QByteArray entryData = dirData.mid(i, 32);
		if (entryData.isEmpty() || (uint8_t)entryData[0] == 0x00)
			break;

		if ((uint8_t)entryData[11] == 0x0F)
		{
			lfnEntries.prepend(entryData);
			continue;
		}

		FatDirEntry entry;
		entry.isDeleted = (uint8_t)entryData[0] == 0xE5;
		entry.isDirectory = (uint8_t)entryData[11] & 0x10;
		QString shortName = QString::fromLatin1(entryData.mid(0, 8)).trimmed();
		QString ext = QString::fromLatin1(entryData.mid(8, 3)).trimmed();
		entry.name = ext.isEmpty() ? shortName : shortName + "." + ext;

		if (!lfnEntries.isEmpty())
		{
			QString longName;
			for (const QByteArray &lfn : lfnEntries)
			{
				for (int j : { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 })
				{
					uint16_t ch = (uint8_t)lfn[j] | (uint8_t)lfn[j + 1] << 8;
					if (ch == 0x0000 || ch == 0xFFFF)
						continue;
					longName.append(QChar(ch));
				}
			}
			entry.name = longName.trimmed();
		}
		entry.modificationTime = readTimeDirectly(entryData, 22);

		QDataStream entryStream(entryData);
		entryStream.setByteOrder(QDataStream::LittleEndian);
		entryStream.device()->seek(28);
		entryStream >> entry.size;
		entry.startCluster =
			bootSector.rootCluster
				? ((uint32_t)(uint8_t)entryData[20] | (uint32_t)(uint8_t)entryData[21] << 8) << 16 |
					  (uint32_t)(uint8_t)entryData[26] | (uint32_t)(uint8_t)entryData[27] << 8
				: (uint32_t)(uint8_t)entryData[26] | (uint32_t)(uint8_t)entryData[27] << 8;

		if (entry.name != "." && entry.name != "..")
		{
			entries.append(entry);
		}
		lfnEntries.clear();
	}
	return entries;
}

QByteArray FatParser::readFile(uint32_t startCluster, uint32_t size)
{
	QByteArray data;
	if (!valid || startCluster < 2)
	{
		return data;
	}

	QList< uint32_t > clusters = getClusterChain(startCluster);
	if (clusters.isEmpty())
	{
		return data;
	}

	uint32_t bytesRead = 0;
	for (uint32_t cluster : clusters)
	{
		QByteArray clusterData = readCluster(cluster);
		if (clusterData.isEmpty())
		{
			return QByteArray();
		}
		data.append(clusterData);
		bytesRead += clusterData.size();

		if (bytesRead >= size)
		{
			break;
		}
	}
	return data.left(size);
}
