#include "outputMessage.h"
#include "mainApp.h"

void CValueOutput::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	m_methodHelper->AppendFunc(wxT("Message"), 1, "Message(str : string)");
}

bool CValueOutput::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
		//--- Специальные:
	case 0:
		static_cast<CCodeRunnerApp*>(wxApp::GetInstance())->AppendOutput(paParams[0]->GetString());
		return true;
	}

	return false;
}

CONTEXT_TYPE_REGISTER(CValueOutput, "ValueOutput", string_to_clsid("IN_OUTP"))