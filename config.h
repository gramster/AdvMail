// config.h - header for the persistent config class
// Written by Graham Wheeler, 1994.
// (c) 1994 by Graham Wheeler, All Rights Reserved.
//
// A simple class for maintaining a persistent set of strings (names and
// values). This was used to restore various config options and variables
// in Gram's Commander 4.0. It has since been replaced by an extension to
// the GCScript Tcl language.

#ifndef _CONFIG_H
#define _CONFIG_H

class PersistentConfig
{
    char *items;
    char **labels;
    char **values;
    int  *flags;
    int eltcnt;
    char *cname; // base name of config file
public:
    PersistentConfig(char *cname_in, char *items_in);
    void Load(char *fname, int merge);
    void Save();
    char *Get(char *label);
    void Set(char *label, char *value);
    int IsSet(char *label);
    ~PersistentConfig();
};

extern PersistentConfig *config;

#endif
