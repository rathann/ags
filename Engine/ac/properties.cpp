//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <stdio.h>
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/properties.h"
#include "ac/string.h"
#include "ac/dynobj/scriptstring.h"
#include "script/runtimescriptvalue.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern GameSetupStruct game;

// begin custom property functions

bool get_property_desc(PropertyDesc &desc, const char *property, PropertyType want_type)
{
    PropertySchema::const_iterator sch_it = game.propSchema.find(property);
    if (sch_it == game.propSchema.end())
        quitprintf("!Did not find property '%s' in the schema. Make sure you are using the property's name, and not its description, when calling this command.", property);

    desc = sch_it->second;
    if (want_type == kPropertyString && desc.Type != kPropertyString)
        quitprintf("!Property '%s' isn't a text property.  Use GetProperty/SetProperty for non-text properties", property);
    else if (want_type != kPropertyString && desc.Type == kPropertyString)
        quitprintf("!Property '%s' is a text property.  Use GetTextProperty/SetTextProperty for text properties", property);
    return true;
}

String get_property_value(const StringIMap &st_prop, const StringIMap &rt_prop, const char *property, const String def_val)
{
    // First check runtime properties, then static properties;
    // if no matching entry was found, use default schema value
    StringIMap::const_iterator it = rt_prop.find(property);
    if (it != rt_prop.end())
        return it->second;
    it = st_prop.find(property);
    if (it != st_prop.end())
        return it->second;
    return def_val;
}

// Get an integer property
int get_int_property(const StringIMap &st_prop, const StringIMap &rt_prop, const char *property)
{
    PropertyDesc desc;
    if (!get_property_desc(desc, property, kPropertyInteger))
        return 0;
    return StrUtil::StringToInt(get_property_value(st_prop, rt_prop, property, desc.DefaultValue));
}

// Get a string property
void get_text_property(const StringIMap &st_prop, const StringIMap &rt_prop, const char *property, char *bufer)
{
    PropertyDesc desc;
    if (!get_property_desc(desc, property, kPropertyString))
        return;

    String val = get_property_value(st_prop, rt_prop, property, desc.DefaultValue);
    snprintf(bufer, MAX_MAXSTRLEN, "%s", val.GetCStr());
}

const char* get_text_property_dynamic_string(const StringIMap &st_prop, const StringIMap &rt_prop, const char *property)
{
    PropertyDesc desc;
    if (!get_property_desc(desc, property, kPropertyString))
        return nullptr;

    String val = get_property_value(st_prop, rt_prop, property, desc.DefaultValue);
    return CreateNewScriptString(val);
}

bool set_int_property(StringIMap &rt_prop, const char *property, int value)
{
    PropertyDesc desc;
    if (get_property_desc(desc, property, kPropertyInteger))
    {
        rt_prop[desc.Name] = StrUtil::IntToString(value);
        return true;
    }
    return false;
}

bool set_text_property(StringIMap &rt_prop, const char *property, const char* value)
{
    PropertyDesc desc;
    if (get_property_desc(desc, property, kPropertyString))
    {
        rt_prop[desc.Name] = value;
        return true;
    }
    return false;
}
