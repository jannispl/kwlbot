#include "StdInc.h"
#include "GlobalModule.h"

#ifdef WIN32
  #define GetLibraryProc(pLibrary, szProc) GetProcAddress(pLibrary, szProc)
#else
#endif

CGlobalModule::CGlobalModule(CCore *pCore, const char *szPath)
{
#ifdef WIN32
	m_pLibrary = LoadLibrary(szPath);
#else
#endif

	if (m_pLibrary == NULL)
	{
		return;
	}

	m_Functions.pfnInitModule = (InitModule_t)GetLibraryProc(m_pLibrary, "InitModule");
	m_Functions.pfnExitModule = (ExitModule_t)GetLibraryProc(m_pLibrary, "ExitModule");
	m_Functions.pfnTemplateRequest = (TemplateRequest_t)GetLibraryProc(m_pLibrary, "TemplateRequest");
	m_Functions.pfnPulse = (Pulse_t)GetLibraryProc(m_pLibrary, "Pulse");

	if (m_Functions.pfnInitModule != NULL)
	{
		char szName[128] = "Unknown";
		if (m_Functions.pfnInitModule(pCore, szName))
		{
			printf("Loaded global module '%s'\n", szName);
		}
		else
		{
			printf("Failed to load global module '%s'\n", szPath);
			FreeLibrary(m_pLibrary);
			m_pLibrary = NULL;
			return;
		}
	}
	else
	{
		printf("Warning: Global module '%s' does not have a \"InitModule\" export!\n", szPath);
	}
}

CGlobalModule::~CGlobalModule()
{
	if (m_pLibrary != NULL)
	{
		if (m_Functions.pfnExitModule != NULL)
		{
			m_Functions.pfnExitModule();
		}

#ifdef WIN32
		FreeLibrary(m_pLibrary);
#else
#endif
	}
}

void CGlobalModule::TemplateRequest(v8::Handle<v8::ObjectTemplate> &objectTemplate)
{
	if (m_Functions.pfnTemplateRequest != NULL)
	{
		m_Functions.pfnTemplateRequest(objectTemplate);
	}
}

void CGlobalModule::Pulse()
{
	if (m_Functions.pfnPulse != NULL)
	{
		m_Functions.pfnPulse();
	}
}
