/// \file
/// \author Xavier Michelon
///
/// \brief Implementation of Combo class that associate a combo text and its substitution text
///  
/// Copyright (c) Xavier Michelon. All rights reserved.  
/// Licensed under the MIT License. See LICENSE file in the project root for full license information.  


#include "stdafx.h"
#include "Combo.h"
#include "InputManager.h"
#include <XMiLib/SystemUtils.h>
#include <XMiLib/Exception.h>


namespace {
   QString const kPropUuid = "uuid"; ///< The JSon property name for the UUID
   QString const kPropName = "name"; ///< The JSON property name for the name
   QString const kPropComboText = "comboText"; ///< The JSON property for the combo text
   QString const kPropSubstitutionText = "substitutionText"; ///< The JSON property name for the substitution text
   QString const kPropCreated = "created"; ///< The JSON property name for the created date/time
   QString const kPropLastModified = "lastModified"; ///< The JSON property name for the modification date/time
   QString const kPropEnabled = "enabled"; ///< The JSON property name for the enabled/disabled state
   Qt::DateFormat const kJsonExportDateFormat = Qt::ISODateWithMs; ///< The date/time export format used for JSon docs
}


using namespace xmilib;


//**********************************************************************************************************************
/// \param[in] name The display name of the combo
/// \param[in] comboText The combo text
/// \param[in] substitutionText The text that will replace the combo
/// \param[in] enabled Is the combo enabled
//**********************************************************************************************************************
Combo::Combo(QString const& name, QString const& comboText, QString const& substitutionText, bool enabled)
   : uuid_(QUuid::createUuid())
   , name_(name)
   , comboText_(comboText)
   , substitutionText_(substitutionText)
   , enabled_(enabled)
{
   lastModified_ = created_ = QDateTime::currentDateTime();
}


//**********************************************************************************************************************
/// If the JSon object is not a valid combo, the constructed combo will be invalid
/// \param[in] object The object to read from
//**********************************************************************************************************************
Combo::Combo(QJsonObject const& object)
   : uuid_(QUuid(object[kPropUuid].toString()))
   , name_(object[kPropName].toString())
   , comboText_(object[kPropComboText].toString())
   , substitutionText_(object[kPropSubstitutionText].toString())
   , created_(QDateTime::fromString(object[kPropCreated].toString(), kJsonExportDateFormat))
   , lastModified_(QDateTime::fromString(object[kPropLastModified].toString(), kJsonExportDateFormat))
   , enabled_(object[kPropEnabled].toBool(true))
{
}


//**********************************************************************************************************************
/// \return true if and only if the combo is valid
//**********************************************************************************************************************
bool Combo::isValid() const
{
   return (!created_.isNull()) && (!lastModified_.isNull()) && (!uuid_.isNull()) && (!name_.isEmpty()) 
      && (!comboText_.isEmpty()) && (!substitutionText_.isEmpty());
}


//**********************************************************************************************************************
/// \return The display name of the combo
//**********************************************************************************************************************
QString Combo::name() const
{
   return name_;
}


//**********************************************************************************************************************
/// \param[in] name The display name of the combo
//**********************************************************************************************************************
void Combo::setName(QString const& name)
{
   if (name_ != name)
   {
      name_ = name;
      this->touch();
   }
}


//**********************************************************************************************************************
/// \return The combo text
//**********************************************************************************************************************
QString Combo::comboText() const
{
   return comboText_;
}


//**********************************************************************************************************************
/// \param[in] comboText The combo text
//**********************************************************************************************************************
void Combo::setComboText(QString const& comboText)
{
   if (comboText_ != comboText)
   {
      comboText_ = comboText;
      this->touch();
   }
}


//**********************************************************************************************************************
/// \return The substitution text
//**********************************************************************************************************************
QString Combo::substitutionText() const
{
   return substitutionText_;
}


//**********************************************************************************************************************
/// \param[in] substitutionText the substitution text
//**********************************************************************************************************************
void Combo::setSubstitutionText(QString const& substitutionText)
{
   if (substitutionText_ != substitutionText)
   {
      substitutionText_ = substitutionText;
      this->touch();
   }
}


//**********************************************************************************************************************
/// \param[in] enabled The value for the enabled parameter
//**********************************************************************************************************************
void Combo::setEnabled(bool enabled)
{
   // Note that enabling / disabling an item does not change its last modification date/time
   enabled_ = enabled;
}


//**********************************************************************************************************************
/// \return true if and only if the combo is enabled
//**********************************************************************************************************************
bool Combo::isEnabled() const
{
   return enabled_;
}


//**********************************************************************************************************************
// 
//*********************************************************************************************************************
void Combo::performSubstitution()
{
   InputManager& inputManager = InputManager::instance();
   bool const wasKeyboardHookWasEnabled = inputManager.setKeyboardHookEnabled(false); // we disable the hook to prevent endless recursive substitution
   try
   {
      // we erase the combo
      synthesizeBackspaces(comboText_.size());

      // we simulate the typing of the substitution text
      for (QChar c : substitutionText_)
      {
         if (c == QChar::LineFeed) // synthesizeUnicode key down does not handle line feed properly (the problem actually comes from Windows API's SendInput())
            synthesizeKeyDownAndUp(VK_RETURN);
         if (!c.isPrint())
            continue;
         synthesizeUnicodeKeyDownAndUp(c.unicode());
      }
      inputManager.setKeyboardHookEnabled(wasKeyboardHookWasEnabled);
   }
   catch (xmilib::Exception const& e)
   {
      inputManager.setKeyboardHookEnabled(wasKeyboardHookWasEnabled);
      throw e;
   }
}


//**********************************************************************************************************************
/// \return A JSon object representing this Combo instance
//**********************************************************************************************************************
QJsonObject Combo::toJsonObject()
{
   QJsonObject result;
   result.insert(kPropUuid, uuid_.toString());
   result.insert(kPropName, name_);
   result.insert(kPropComboText, comboText_);
   result.insert(kPropSubstitutionText, substitutionText_);
   result.insert(kPropCreated, created_.toString(kJsonExportDateFormat));
   result.insert(kPropLastModified, lastModified_.toString(kJsonExportDateFormat));
   result.insert(kPropEnabled, enabled_);
   return result;
}

//**********************************************************************************************************************
/// \param[in] name The display name of the combo
/// \param[in] comboText The combo text
/// \param[in] substitutionText The text that will replace the combo
/// \param[in] enabled Is the combo enabled
/// \return A shared pointer to the created Combo
//**********************************************************************************************************************
SPCombo Combo::create(QString const& name, QString const& comboText, QString const& substitutionText, bool enabled)
{
   return std::make_shared<Combo>(name, comboText, substitutionText, enabled);
}


//**********************************************************************************************************************
/// If the JSon object is not a valid combo, the constructed combo will be invalid
///
/// \param[in] object The object to read from
/// \return A shared pointer to the created Combo
//**********************************************************************************************************************
SPCombo Combo::create(QJsonObject const& object)
{
   return std::make_shared<Combo>(object);
}


//**********************************************************************************************************************
/// Note that we make a distinction between a copy that can be generated by a copy constructor or assignment operator
/// and a duplicate: a duplicate has a different UUID and as such, even if all other field are strictly identical
/// a duplicate and its original are to be considered as different.
///
/// Also note that the combo text is not copied as two combos cannot have the same combo text.
///
/// \return a shared pointer pointing to a duplicate of the combo
//**********************************************************************************************************************
SPCombo Combo::duplicate(Combo const& combo)
{
   // note that the duplicate is enabled even if the source is not.
   return std::make_shared<Combo>(combo.name(), QString(), combo.substitutionText());
}


//**********************************************************************************************************************
/// This function is named after the UNIX touch command.
//**********************************************************************************************************************
void Combo::touch()
{
   lastModified_ = QDateTime::currentDateTime();
}

