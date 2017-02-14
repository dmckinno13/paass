/** \file DetectorLibrary.cpp
 *  \brief Some useful function for managing the list of channel identifiers
 *  \author David Miller
 */

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <string>

#include "pugixml.hpp"

#include "DetectorLibrary.hpp"
#include "MapNodeXmlParser.hpp"
#include "Messenger.hpp"
#include "TreeCorrelator.hpp"

using namespace std;

set<int> DetectorLibrary::emptyLocations;

DetectorLibrary* DetectorLibrary::instance = NULL;

DetectorLibrary* DetectorLibrary::get() {
    if (!instance)
        instance = new DetectorLibrary();
    return(instance);
}

DetectorLibrary::DetectorLibrary() : vector<Identifier>(), locations(),
    numModules(0) {
    MapNodeXmlParser parser;
    parser.ParseNode(this);

    /* At this point basic Correlator places build automatically from
     * map file should be created so we can call buildTree function */
    ///@TODO this needs to be moved out of this constructor and into a
    /// location that's more fitting.
    try {
        TreeCorrelator::get()->buildTree();
    } catch (exception &e) {
        cout << "Exception caught in DetectorLibrary" << endl;
        cout << "\t" << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}

DetectorLibrary::const_reference DetectorLibrary::at(DetectorLibrary::size_type idx) const {
    return vector<Identifier>::at(idx);
}

DetectorLibrary::const_reference DetectorLibrary::at(DetectorLibrary::size_type mod,
                                                     DetectorLibrary::size_type ch) const {
    return vector<Identifier>::at(GetIndex(mod,ch));
}

DetectorLibrary::reference DetectorLibrary::at(DetectorLibrary::size_type idx) {
    return vector<Identifier>::at(idx);
}

DetectorLibrary::reference DetectorLibrary::at(DetectorLibrary::size_type mod,
                                               DetectorLibrary::size_type ch) {
    return vector<Identifier>::at(GetIndex(mod,ch));
}


void DetectorLibrary::push_back(const Identifier &x) {
    mapkey_t key = MakeKey(x.GetType(), x.GetSubtype());

    locations[key].insert(x.GetLocation());
    vector<Identifier>::push_back(x);
}

const set<int>& DetectorLibrary::GetLocations(const Identifier &id) const {
    return GetLocations(id.GetType(), id.GetSubtype());
}

const set<int>& DetectorLibrary::GetLocations(const std::string &type,
                                              const std::string &subtype) const{
    mapkey_t key = MakeKey(type, subtype);

    if (locations.count(key) > 0) {
        return locations.find(key)->second;
    } else {
        return emptyLocations;
    }
}

int DetectorLibrary::GetNextLocation(const Identifier &id) const {
  return GetNextLocation(id.GetType(), id.GetSubtype());
}

int DetectorLibrary::GetNextLocation(const std::string &type,
				     const std::string &subtype) const {
    mapkey_t key = MakeKey(type, subtype);

    if (locations.count(key) > 0) {
        return *(locations.find(key)->second.rbegin()) + 1;
    } else {
        return 0;
    }
}

DetectorLibrary::size_type DetectorLibrary::GetIndex(int mod, int chan) const {
  return mod * Pixie16::maximumNumberOfChannels + chan;
}

bool DetectorLibrary::HasValue(int mod, int chan) const {
    return HasValue(GetIndex(mod,chan));
}

bool DetectorLibrary::HasValue(int index) const {
  return ((signed)size() > index && at(index).GetType() != "");
}

void DetectorLibrary::Set(int index, const Identifier& value) {
    if (knownDetectors.find(value.GetType()) == knownDetectors.end())
        knownDetectors.insert(value.GetType());

    unsigned int module = ModuleFromIndex(index);
    if (module >= numModules ) {
        numModules = module + 1;
        resize(numModules * Pixie16::maximumNumberOfChannels);
        if (!value.HasTag("virtual")) {
            numPhysicalModules = module + 1;
        }
    }

    string key;
    key = value.GetType() + ':' + value.GetSubtype();
    locations[key].insert(value.GetLocation());

    usedTypes.insert(value.GetType());
    usedSubtypes.insert(value.GetSubtype());

    at(index) = value;
}

void DetectorLibrary::Set(int mod, int ch, const Identifier &value) {
    Set(GetIndex(mod,ch), value);
}

void DetectorLibrary::PrintMap(void) const {
    cout << setw(4) << "MOD"
	 << setw(4) << "CH";
    Identifier::PrintHeaders();

    for (size_t i=0; i < size(); i++) {
        cout << setw(4) << ModuleFromIndex(i)
            << setw(4) << ChannelFromIndex(i);
        at(i).Print();
    }
}

void DetectorLibrary::PrintUsedDetectors(RawEvent& rawev) const {
    Messenger m;
    stringstream ss;
    ss << usedTypes.size() << " detector types are used in this analysis "
	   << "and are named:";
    m.detail(ss.str());
    ss.str("");
    copy(usedTypes.begin(), usedTypes.end(),
         ostream_iterator<string>(ss, " "));
    m.detail(ss.str(), 1);
    ss.str("");

    ss << usedSubtypes.size() <<" detector subtypes are used in this "
       << "analysis and are named:";
    m.detail(ss.str());
    ss.str("");
    copy(usedSubtypes.begin(), usedSubtypes.end(),
         ostream_iterator<string>(ss," "));
    m.detail(ss.str(), 1);

    rawev.Init(usedTypes);
}

const set<string>& DetectorLibrary::GetUsedDetectors(void) const {
    return usedTypes;
}

int DetectorLibrary::ModuleFromIndex(int index) const {
    return int(index / Pixie16::maximumNumberOfChannels);
}

int DetectorLibrary::ChannelFromIndex(int index) const {
    return (index % Pixie16::maximumNumberOfChannels);
}

DetectorLibrary::mapkey_t DetectorLibrary::MakeKey(const std::string &type,
                                                   const std::string &subtype) const {
    return (type + ':' + subtype);
}

DetectorLibrary::~DetectorLibrary() {
    delete instance;
    instance = NULL;
}

