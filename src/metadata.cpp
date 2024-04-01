/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 myst6re                                            //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2024 Julian Xhokaxhiu                                   //
//                                                                          //
//    This file is part of FFNx                                             //
//                                                                          //
//    FFNx is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU General Public License as published by  //
//    the Free Software Foundation, either version 3 of the License         //
//                                                                          //
//    FFNx is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of        //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
//    GNU General Public License for more details.                          //
/****************************************************************************/

#include "metadata.h"

Metadata metadataPatcher;

// PRIVATE
void Metadata::loadXml()
{
  ffnx_trace("Metadata: loading metadata.xml\n");

  // Load Metadata
  doc.load_file(savePath);
}

void Metadata::saveXml()
{
  ffnx_trace("Metadata: saving %s\n", savePath);

  // Save Metadata
  doc.save_file(savePath);
}

void Metadata::calcNow()
{
  std::chrono::milliseconds nowMS = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
  );

  now = std::to_string(nowMS.count());
}

// PUBLIC
void Metadata::init()
{
  ffnx_trace("Metadata: Initializing manager.\n");

  // Get Save Path
  get_userdata_path(userPath, sizeof(userPath), true);

  // Get Metadata Path
  strcpy(savePath, userPath);
  PathAppendA(savePath, "metadata.xml");

  // Save userID
  userID.assign(strrchr(userPath, '_') + 1);
}

std::string Metadata::hashFile(const char* filePath, size_t bufferSize)
{
  BYTE* dataBuffer = new BYTE[bufferSize + userID.length()];
  int dataSize = userID.length();
  FILE* file = fopen(filePath, "rb");

  if (file)
  {
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    fread(dataBuffer, 1, fileSize, file);
    fclose(file);

    memcpy(dataBuffer + fileSize, userID.data(), userID.length());
    dataSize += fileSize;
  }
  else
  {
    memcpy(dataBuffer, userID.data(), userID.length());
  }

  MD5 md5(dataBuffer, dataSize);
  delete[] dataBuffer;

  return md5.hexdigest();
}

std::string Metadata::emptyHash()
{
  int dataSize = userID.length();
  BYTE* dataBuffer = new BYTE[dataSize];

  memcpy(dataBuffer, userID.data(), dataSize);
  MD5 md5(dataBuffer, dataSize);
  delete[] dataBuffer;

  return md5.hexdigest();
}

void Metadata::updateFF7(uint8_t slot, uint8_t save)
{
  char currentSave[260]{ 0 };
  size_t bufferSize = 64 * 1024 + 8;

  // Append save file name
  strcpy(currentSave, userPath);
  sprintf(currentSave + strlen(currentSave), R"(\save%02i.ff7)", save);

  loadXml();
  calcNow();

  // Check if <gamestatus> exists and if not, create it
  if (doc.child("gamestatus") == nullptr)
  {
    ffnx_trace("Metadata: creating <gamestatus> node\n");

    std::string hash = emptyHash();
    pugi::xml_node gamestatus = doc.append_child("gamestatus");

    for (int i = 0; i < 10; i++)
    {
      pugi::xml_node savefiles = gamestatus.append_child("savefiles");
      savefiles.append_attribute("block").set_value(i + 1);

      for (int j = 0; j < 15; j++)
      {
        pugi::xml_node timestamp = savefiles.append_child("timestamp");
        timestamp.append_attribute("slot").set_value(j + 1);
        timestamp.text().set(now.data());
      }
      pugi::xml_node signature = savefiles.append_child("signature");
      signature.text().set(hash.data());
    }
  }

  std::string hash = hashFile(currentSave, bufferSize);

  // Update metadata
  for (pugi::xml_node gamestatus : doc.children())
  {
    for (pugi::xml_node savefiles : gamestatus.children())
    {
      if (std::atoi(savefiles.attribute("block").value()) == (save + 1))
      {
        ffnx_trace("Metadata: updating timestamp and signature for %s\n", currentSave);

        for (pugi::xml_node child : savefiles.children())
        {
          if (strcmp(child.name(), "timestamp") == 0 && std::atoi(child.attribute("slot").value()) == slot + 1)
          {
            child.text().set(now.data());
          }

          if (strcmp(child.name(), "signature") == 0)
          {
            child.text().set(hash.data());
          }
        }
      }
    }
  }

  // Flush
  saveXml();
}

void Metadata::updateFF8(uint8_t slot, uint8_t save)
{
  char currentSave[260]{ 0 };
  size_t bufferSize = 8 * 1024 + 8;

  // Append save file name
  strcpy(currentSave, userPath);
  if (slot > 2) {
    strcpy(currentSave + strlen(currentSave), "\\chocorpg.ff8");
  }
  else {
    sprintf(currentSave + strlen(currentSave), R"(\slot%d_save%02i.ff8)", slot, save);
  }

  loadXml();
  calcNow();

  std::string hash = hashFile(currentSave, bufferSize);

  // Update metadata
  for (pugi::xml_node gamestatus : doc.children())
  {
    for (pugi::xml_node savefile : gamestatus.children())
    {
      if ((
        strcmp(savefile.attribute("type").value(), "choco") == 0 && slot > 2) ||
        (std::atoi(savefile.attribute("num").value()) == save && std::atoi(savefile.attribute("slot").value()) == slot)
        )
      {
        ffnx_trace("Metadata: updating timestamp and signature for %s\n", currentSave);

        for (pugi::xml_node child : savefile.children())
        {
          if (strcmp(child.name(), "timestamp") == 0)
          {
            child.text().set(now.data());
          }

          if (strcmp(child.name(), "signature") == 0)
          {
            child.text().set(hash.data());
          }
        }
      }
    }
  }

  // Flush
  saveXml();
}
