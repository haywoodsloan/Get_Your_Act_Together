#include "Ws2tcpip.h"
#include "Fwpmu.h"
#include "SiteBlocking.h"

HANDLE engineHandle = 0;
FWPM_SUBLAYER0 subLayer;
FWPM_FILTER0 blockSiteFilterI;
FWPM_FILTER0 blockSiteFilterO;
FWPM_FILTER0 blockSiteFilterV6I;
FWPM_FILTER0 blockSiteFilterV6O;

vector<u_long> siteLongs;
vector<UINT8*> siteUINTS;

unsigned int sizeLongs;
unsigned int sizeUINTS;

bool containsLong(u_long tempAddr){
	for (unsigned int i = 0; i < siteLongs.size(); i++){

		if (siteLongs[i] == tempAddr){
			return true;
		}
	}

	return false;
}

void getAddrList(vector<char*> siteList)
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	addrinfo *info = NULL;

	for (unsigned int i = 0; i < siteList.size(); i++){

		getaddrinfo(siteList[i], NULL, NULL, &info);

		if (info != NULL){

			addrinfo *tempInfo = info;
			while (tempInfo != NULL){

				if (tempInfo->ai_family == AF_INET){

					u_long tempAddr = htonl(((sockaddr_in*)tempInfo->ai_addr)->sin_addr.S_un.S_addr);
					if (!containsLong(tempAddr)){
						siteLongs.push_back(tempAddr);
					}
				}
				tempInfo = tempInfo->ai_next;
			}
		}

		freeaddrinfo(info);
		
		string modifiedString = "www." + string(siteList[i]);

		getaddrinfo(modifiedString.c_str(), NULL, NULL, &info);

		if (info != NULL){

			addrinfo *tempInfo = info;
			while (tempInfo != NULL){

				if (tempInfo->ai_family == AF_INET){

					u_long tempAddr = htonl(((sockaddr_in*)tempInfo->ai_addr)->sin_addr.S_un.S_addr);
					if (!containsLong(tempAddr)){
						siteLongs.push_back(tempAddr);
					}
				}
				tempInfo = tempInfo->ai_next;
			}
		}

		freeaddrinfo(info);
	}
}

bool containsUINT(UINT8* tempAddr){
	for (unsigned int i = 0; i < siteUINTS.size(); i++){

		bool matches = true;

		for (int i2 = 0; i2 < 16; i2++){

			if (siteUINTS[i][i2] != tempAddr[i2]){
				matches = false;
			}
		}

		if (matches){
			return true;
		}
	}

	return false;
}

void getAddrListV6(vector<char*> siteList)
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	addrinfo *info = NULL;

	for (unsigned int i = 0; i < siteList.size(); i++){

		getaddrinfo(siteList[i], NULL, NULL, &info);

		if (info != NULL){

			addrinfo *tempInfo = info;
			while (tempInfo != NULL){

				if (tempInfo->ai_family == AF_INET6){

					UINT8 *tempAddr = new UINT8[16];
					for (int i2 = 0; i2 < 16; i2++){
						tempAddr[i2] = ((sockaddr_in6*)tempInfo->ai_addr)->sin6_addr.u.Byte[i2];
					}

					if (!containsUINT(tempAddr)){
						siteUINTS.push_back(tempAddr);
					}
				}
				tempInfo = tempInfo->ai_next;
			}
		}

		freeaddrinfo(info);
		
		string modifiedString = "www." + string(siteList[i]);

		getaddrinfo(modifiedString.c_str(), NULL, NULL, &info);

		if (info != NULL){

			addrinfo *tempInfo = info;

			while (tempInfo != NULL){

				if (tempInfo->ai_family == AF_INET6){

					UINT8 *tempAddr = new UINT8[16];
					for (int i2 = 0; i2 < 16; i2++){
						tempAddr[i2] = ((sockaddr_in6*)tempInfo->ai_addr)->sin6_addr.u.Byte[i2];
					}

					if (!containsUINT(tempAddr)){
						siteUINTS.push_back(tempAddr);
					}
				}
				tempInfo = tempInfo->ai_next;
			}
		}

		freeaddrinfo(info);
	}
}

void blockSites(vector<char*> siteList)
{
	getAddrList(siteList);
	getAddrListV6(siteList);

	if (sizeLongs < siteLongs.size() || sizeUINTS < siteUINTS.size()) {

		sizeLongs = siteLongs.size();
		sizeUINTS = siteUINTS.size();

		HANDLE oldEngineHandle = 0;
		GUID oldSubLayerKey;
		UINT64 oldBlockSiteFilterIID;
		UINT64 oldBlockSiteFilterOID;
		UINT64 oldBlockSiteFilterV6IID;
		UINT64 oldBlockSiteFilterV6OID;

		if (engineHandle != 0) {
			oldEngineHandle = engineHandle;
			oldSubLayerKey = subLayer.subLayerKey;
			oldBlockSiteFilterIID = blockSiteFilterI.filterId;
			oldBlockSiteFilterOID = blockSiteFilterO.filterId;
			oldBlockSiteFilterV6IID = blockSiteFilterV6I.filterId;
			oldBlockSiteFilterV6OID = blockSiteFilterV6O.filterId;
		}

		UINT32 status = ERROR_SUCCESS;
		FWPM_FILTER_CONDITION0 *filterConditions = new FWPM_FILTER_CONDITION0[siteLongs.size()];
		FWP_V4_ADDR_AND_MASK *addrAndMasks = new FWP_V4_ADDR_AND_MASK[siteLongs.size()];
		FWPM_FILTER_CONDITION0 *filterConditionsV6 = new FWPM_FILTER_CONDITION0[siteUINTS.size()];
		FWP_V6_ADDR_AND_MASK *addrAndMasksV6 = new FWP_V6_ADDR_AND_MASK[siteUINTS.size()];

		ZeroMemory(&subLayer, sizeof(FWPM_SUBLAYER0));
		ZeroMemory(&blockSiteFilterI, sizeof(FWPM_FILTER0));
		ZeroMemory(&blockSiteFilterO, sizeof(FWPM_FILTER0));
		ZeroMemory(&blockSiteFilterV6I, sizeof(FWPM_FILTER0));
		ZeroMemory(&blockSiteFilterV6O, sizeof(FWPM_FILTER0));
		ZeroMemory(filterConditions, sizeof(FWPM_FILTER_CONDITION0)*siteLongs.size());
		ZeroMemory(addrAndMasks, sizeof(FWP_V4_ADDR_AND_MASK)*siteLongs.size());
		ZeroMemory(filterConditionsV6, sizeof(FWPM_FILTER_CONDITION0)*siteUINTS.size());
		ZeroMemory(addrAndMasksV6, sizeof(FWP_V6_ADDR_AND_MASK)*siteUINTS.size());

		status = UuidCreate(&(subLayer.subLayerKey));

		subLayer.displayData.name = L"Get Your Act Together";
		subLayer.displayData.description = L"Get Your Act Together Sublayer";

		for (unsigned int i = 0; i < siteLongs.size(); i++) {

			addrAndMasks[i].addr = siteLongs[i];
			addrAndMasks[i].mask = 0xFFFFFFFF;

			filterConditions[i].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
			filterConditions[i].matchType = FWP_MATCH_EQUAL;
			filterConditions[i].conditionValue.type = FWP_V4_ADDR_MASK;
			filterConditions[i].conditionValue.v4AddrMask = &addrAndMasks[i];
		}

		for (unsigned int i = 0; i < siteUINTS.size(); i++) {

			for (int i2 = 0; i2 < 16; i2++) {
				addrAndMasksV6[i].addr[i2] = siteUINTS[i][i2];
			}

			addrAndMasksV6[i].prefixLength = 128;

			filterConditionsV6[i].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
			filterConditionsV6[i].matchType = FWP_MATCH_EQUAL;
			filterConditionsV6[i].conditionValue.type = FWP_V6_ADDR_MASK;
			filterConditionsV6[i].conditionValue.v6AddrMask = &addrAndMasksV6[i];
		}

		blockSiteFilterI.subLayerKey = subLayer.subLayerKey;
		blockSiteFilterI.displayData.name = L"Filter to block inbound ipv4 traffic to sites given as argument";
		blockSiteFilterI.layerKey = FWPM_LAYER_INBOUND_IPPACKET_V4;
		blockSiteFilterI.action.type = FWP_ACTION_BLOCK;
		blockSiteFilterI.filterCondition = filterConditions;
		blockSiteFilterI.numFilterConditions = siteLongs.size();
		blockSiteFilterI.weight.type = FWP_UINT8;
		blockSiteFilterI.weight.uint8 = 0x00;

		blockSiteFilterO.subLayerKey = subLayer.subLayerKey;
		blockSiteFilterO.displayData.name = L"Filter to block outbound ipv4 traffic to sites given as argument";
		blockSiteFilterO.layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V4;
		blockSiteFilterO.action.type = FWP_ACTION_BLOCK;
		blockSiteFilterO.filterCondition = filterConditions;
		blockSiteFilterO.numFilterConditions = siteLongs.size();
		blockSiteFilterO.weight.type = FWP_UINT8;
		blockSiteFilterO.weight.uint8 = 0x00;

		blockSiteFilterV6I.subLayerKey = subLayer.subLayerKey;
		blockSiteFilterV6I.displayData.name = L"Filter to block inbound ipv6 traffic to sites given as argument";
		blockSiteFilterV6I.layerKey = FWPM_LAYER_INBOUND_IPPACKET_V6;
		blockSiteFilterV6I.action.type = FWP_ACTION_BLOCK;
		blockSiteFilterV6I.filterCondition = filterConditionsV6;
		blockSiteFilterV6I.numFilterConditions = siteUINTS.size();
		blockSiteFilterV6I.weight.type = FWP_UINT8;
		blockSiteFilterV6I.weight.uint8 = 0x01;

		blockSiteFilterV6O.subLayerKey = subLayer.subLayerKey;
		blockSiteFilterV6O.displayData.name = L"Filter to block outbound ipv6 traffic to sites given as argument";
		blockSiteFilterV6O.layerKey = FWPM_LAYER_OUTBOUND_IPPACKET_V6;
		blockSiteFilterV6O.action.type = FWP_ACTION_BLOCK;
		blockSiteFilterV6O.filterCondition = filterConditionsV6;
		blockSiteFilterV6O.numFilterConditions = siteUINTS.size();
		blockSiteFilterV6O.weight.type = FWP_UINT8;
		blockSiteFilterV6O.weight.uint8 = 0x01;

		status = FwpmEngineOpen0(0, RPC_C_AUTHN_WINNT, 0, 0, &engineHandle);
		status = FwpmSubLayerAdd0(engineHandle, &subLayer, 0);
		status = FwpmFilterAdd0(engineHandle, &blockSiteFilterI, 0, &(blockSiteFilterI.filterId));
		status = FwpmFilterAdd0(engineHandle, &blockSiteFilterO, 0, &(blockSiteFilterO.filterId));
		status = FwpmFilterAdd0(engineHandle, &blockSiteFilterV6I, 0, &(blockSiteFilterV6I.filterId));
		status = FwpmFilterAdd0(engineHandle, &blockSiteFilterV6O, 0, &(blockSiteFilterV6O.filterId));

		delete[](filterConditions);
		delete[](addrAndMasks);
		delete[](filterConditionsV6);
		delete[](addrAndMasksV6);

		removeFilter(oldEngineHandle, &oldSubLayerKey, oldBlockSiteFilterIID, oldBlockSiteFilterOID, oldBlockSiteFilterV6IID, oldBlockSiteFilterV6OID);
	}
}

UINT32 removeFilter()
{
	UINT32  status = ERROR_SUCCESS;

	status = FwpmFilterDeleteById0(engineHandle, blockSiteFilterI.filterId);
	status = FwpmFilterDeleteById0(engineHandle, blockSiteFilterO.filterId);
	status = FwpmFilterDeleteById0(engineHandle, blockSiteFilterV6I.filterId);
	status = FwpmFilterDeleteById0(engineHandle, blockSiteFilterV6O.filterId);
	status = FwpmSubLayerDeleteByKey0(engineHandle, &(subLayer.subLayerKey));
	status = FwpmEngineClose0(engineHandle);

	engineHandle = 0;

	return status;
}

UINT32 removeFilter(HANDLE oldEngineHandle,
	GUID *oldSubLayerKey,
	UINT64 oldBlockSiteFilterIID,
	UINT64 oldBlockSiteFilterOID,
	UINT64 oldBlockSiteFilterV6IID,
	UINT64 oldBlockSiteFilterV6OID) {

	UINT32  status = ERROR_SUCCESS;

	status = FwpmFilterDeleteById0(oldEngineHandle, oldBlockSiteFilterIID);
	status = FwpmFilterDeleteById0(oldEngineHandle, oldBlockSiteFilterOID);
	status = FwpmFilterDeleteById0(oldEngineHandle, oldBlockSiteFilterV6IID);
	status = FwpmFilterDeleteById0(oldEngineHandle, oldBlockSiteFilterV6OID);
	status = FwpmSubLayerDeleteByKey0(oldEngineHandle, oldSubLayerKey);
	status = FwpmEngineClose0(oldEngineHandle);

	return status;
}