#include "sysreg.h"

bool GetConsole(virDomainPtr vDomPtr, char* console)
{
    xmlDocPtr xml = NULL;
    xmlXPathObjectPtr obj = NULL;
    xmlXPathContextPtr ctxt = NULL;
    char* XmlDoc;
    bool RetVal = false;

    XmlDoc = virDomainGetXMLDesc(vDomPtr, 0);
    if (!XmlDoc)
        return false;

    xml = xmlReadDoc((const xmlChar *) XmlDoc, "domain.xml", NULL,
            XML_PARSE_NOENT | XML_PARSE_NONET |
            XML_PARSE_NOWARNING);
    free(XmlDoc);
    if (!xml)
        return false;

    ctxt = xmlXPathNewContext(xml);
    if (!ctxt)
    {
        xmlFreeDoc(xml);
        return false;
    }

    obj = xmlXPathEval(BAD_CAST "string(/domain/devices/console/@tty)", ctxt);
    if ((obj != NULL) && ((obj->type == XPATH_STRING) &&
                         (obj->stringval != NULL) && (obj->stringval[0] != 0)))
    {
        strcpy(console, obj->stringval);
        RetVal = true;
    }
    if (obj)
        xmlXPathFreeObject(obj);

    xmlFreeDoc(xml);
    xmlXPathFreeContext(ctxt);
    return RetVal;
}

bool IsVirtualMachineRunning(virConnectPtr vConn, const char* name)
{
    int* ids = NULL;
    int numids;
    int maxids = 0;
    const char* domname;
    virDomainPtr vDomPtr = NULL;

    maxids = virConnectNumOfDomains(vConn);
    if (maxids < 0)
        return false;

    ids = malloc(sizeof(int) * maxids);
    if (!ids)
        return false;
    
    numids = virConnectListDomains(vConn, &ids[0], maxids);
    if (numids > -1)    
    {
        int i;
        for(i=0; i<numids; i++)
        {
            vDomPtr = virDomainLookupByID(vConn, ids[i]);
            domname = virDomainGetName(vDomPtr);
            if (strcasecmp(name, domname) == 0)
            {
                virDomainFree(vDomPtr);
                free(ids);
                return true; 
            }
            virDomainFree(vDomPtr);
        }
    }
    free(ids);
    return false;
}

virDomainPtr LaunchVirtualMachine(virConnectPtr vConn, const char* XmlFileName, const char* BootDevice)
{
    xmlDocPtr xml = NULL;
    xmlXPathObjectPtr obj = NULL;
    xmlXPathContextPtr ctxt = NULL;
    char* XmlDoc;
    char* buffer;
    const char* name;
    char* domname;
    int len = 0;

    buffer = ReadFile(XmlFileName);
    if (buffer == NULL)
        return NULL;

    xml = xmlReadDoc((const xmlChar *) buffer, "domain.xml", NULL,
                      XML_PARSE_NOENT | XML_PARSE_NONET |
                      XML_PARSE_NOWARNING);
    if (!xml)
        return NULL;

    ctxt = xmlXPathNewContext(xml);
    if (!ctxt)
        return NULL;

    obj = xmlXPathEval(BAD_CAST "/domain/os/boot", ctxt);
    if ((obj != NULL) && (obj->type == XPATH_NODESET) 
            && (obj->nodesetval != NULL) && (obj->nodesetval->nodeTab != NULL)) 
    {
        xmlSetProp(obj->nodesetval->nodeTab[0], "dev", BootDevice);
    }
    if (obj)
        xmlXPathFreeObject(obj);

    free(buffer);
    xmlDocDumpMemory(xml, (xmlChar**) &buffer, &len);
    xmlFreeDoc(xml);
    xmlXPathFreeContext(ctxt);

    virDomainPtr vDomPtr = virDomainDefineXML(vConn, buffer);
    xmlFree((xmlChar*)buffer);
    if (vDomPtr)
    {
        if (virDomainCreate(vDomPtr) != 0)
        {
            virDomainUndefine(vDomPtr);
            vDomPtr = NULL;
        }
        else
        {
            /* workaround a bug in libvirt */
            name = virDomainGetName(vDomPtr);
            domname = strdup(name);
            virDomainFree(vDomPtr);
            vDomPtr = virDomainLookupByName(vConn, domname);
            free(domname);
        }
    }
    return vDomPtr;
}


int main(int argc, char **argv)
{
    virConnectPtr vConn = NULL;
    virDomainPtr vDom;
    virDomainInfo info;
	int Crashes;
    int Stage;
    char qemu_img_cmdline[300];
    FILE* file;
    char config[255];
    int Ret = EXIT_NONCONTINUABLE_ERROR;
    char console[50];

    if (argc == 2)
        strcpy(config, argv[1]);
    else
        strcpy(config, "sysreg.xml");

    if (!LoadSettings(config))
    {
        SysregPrintf("Cannot load configuration file\n");
        goto cleanup;
    }
    vConn = virConnectOpen("qemu:///session");

    if (IsVirtualMachineRunning(vConn, AppSettings.Name))
    {
        SysregPrintf("Error: Virtual Machine is already running.\n");
        goto cleanup;
    }

    /* If the HD image already exists, delete it */
    if (file = fopen(AppSettings.HardDiskImage, "r"))
    {
        fclose(file);
        remove(AppSettings.HardDiskImage);
    }

    /* Create a new HD image */
    sprintf(qemu_img_cmdline, "qemu-img create -f qcow2 %s %dM", 
            AppSettings.HardDiskImage, AppSettings.ImageSize);
    FILE* p = popen(qemu_img_cmdline, "r");
    char buf[100];
    while(feof(p)==0)
    {
        fgets(buf,100,p);
        SysregPrintf("%s\n",buf);
    }
    pclose(p);

    for(Stage = 0; Stage < NUM_STAGES; Stage++)
    {
        for(Crashes = 0; Crashes < AppSettings.MaxCrashes; Crashes++)
        {
            vDom = LaunchVirtualMachine(vConn, AppSettings.Filename,
                    AppSettings.Stage[Stage].BootDevice);

            if (!vDom)
            {
                SysregPrintf("LaunchVirtualMachine failed!\n");
                goto cleanup;
            }

            printf("\n\n\n");
            SysregPrintf("Running stage %d...\n", Stage + 1);
            SysregPrintf("Domain %s started.\n", virDomainGetName(vDom));

            GetConsole(vDom, console);
            Ret = ProcessDebugData(console, AppSettings.Timeout, Stage);

            /* Kill the VM */
            virDomainGetInfo(vDom, &info);

            if (info.state != VIR_DOMAIN_SHUTOFF)
                virDomainDestroy(vDom);

            virDomainUndefine(vDom);
            virDomainFree(vDom);

            /* If we have a checkpoint to reach for success, assume that
               the application used for running the tests (probably "rosautotest")
               continues with the next test after a VM restart. */
            if (Ret == EXIT_ERROR && *AppSettings.Stage[Stage].Checkpoint)
                SysregPrintf("Crash %d encountered, resuming the testing process\n", Crashes);
            else
                break;
        }

        if (Crashes == AppSettings.MaxCrashes)
        {
            SysregPrintf("Maximum number of allowed crashes exceeded, aborting!\n");
            break;
        }

        if (Ret == EXIT_ERROR || Ret == EXIT_NONCONTINUABLE_ERROR)
            break;
    }


cleanup:
    if (vConn)
        virConnectClose(vConn);

    switch (Ret)
    {
        case EXIT_CHECKPOINT_REACHED:
            SysregPrintf("Status: Reached the checkpoint!\n");
            break;

        case EXIT_SHUTDOWN:
            SysregPrintf("Status: Machine shut down, but did not reach the checkpoint!\n");
            break;

        case EXIT_ERROR:
            SysregPrintf("Status: Failed to reach the checkpoint!\n");
            break;

        case EXIT_NONCONTINUABLE_ERROR:
            SysregPrintf("Status: Testing process aborted!\n");
            break;
    }

    return Ret;
}
