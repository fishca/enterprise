////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : frame control
////////////////////////////////////////////////////////////////////////////

#include "form.h"
#include "backend/metaCollection/partial/commonObject.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueForm, IValueFrame);

//****************************************************************************
//*                              Frame                                       *
//****************************************************************************

CValueForm::CValueForm(const IValueMetaObjectForm* creator, IControlFrame* ownerControl,
	ISourceDataObject* srcObject, const CUniqueKey& formGuid) : IValueFrame(), IModuleDataObject(),
	m_controlOwner(nullptr), m_sourceObject(nullptr), m_metaFormObject(nullptr),
	m_formCollectionControl(CValue::CreateAndPrepareValueRef<CValueFormCollectionControl>(this)),
	m_formType(defaultFormType), m_closeOnChoice(true), m_closeOnOwnerClose(true), m_formModified(false)
{
	//init default params
	CValueForm::InitializeForm(creator, ownerControl, srcObject, formGuid);

	//set default params
	m_controlId = defaultFormId;
}

CValueForm::~CValueForm()
{
	for (auto pair : m_idleHandlerArray) {
		wxTimer* timer = pair.second;
		if (timer->IsRunning()) {
			timer->Stop();
		}
		timer->Unbind(wxEVT_TIMER, &CValueForm::OnIdleHandler, this);
		delete timer;
	}

	for (unsigned int idx = GetChildCount(); idx > 0; idx--) {
		IValueFrame* controlChild = GetChild(idx - 1);
		ClearRecursive(controlChild);
		if (controlChild != nullptr) controlChild->DecrRef();
	}

	if (m_controlOwner != nullptr) m_controlOwner->ControlDecrRef();
	if (m_sourceObject != nullptr) m_sourceObject->SourceDecrRef();
}

void CValueForm::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	UpdateForm();
}

void CValueForm::OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost)
{
	//lay out parent window 
	wxWindow* wndParent = visualHost->GetParent();
	if (wndParent) {
		wndParent->Layout();
	}
}

//**********************************************************************************
//*                                   Data		                                   *
//**********************************************************************************

bool CValueForm::LoadData(CMemoryReader& reader)
{
	wxString propValue = wxEmptyString;
	reader.r_stringZ(propValue);
	m_propertyTitle->SetValue(propValue);
	m_propertyOrient->SetValue(reader.r_s32());
	reader.r_stringZ(propValue);
	m_propertyFG->SetValue(typeConv::StringToColour(propValue));
	reader.r_stringZ(propValue);
	m_propertyBG->SetValue(typeConv::StringToColour(propValue));
	m_propertyEnabled->SetValue((bool)reader.r_u8());
	return IValueFrame::LoadData(reader);
}

bool CValueForm::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(m_propertyTitle->GetValueAsString());
	writer.w_s32(m_propertyOrient->GetValueAsInteger());

	writer.w_stringZ(
		m_propertyFG->GetValueAsString()
	);
	writer.w_stringZ(
		m_propertyBG->GetValueAsString()
	);
	writer.w_u8(m_propertyEnabled->GetValueAsBoolean());
	return IValueFrame::SaveData(writer);
}

//**********************************************************************************
//*                                   Other                                        *
//**********************************************************************************

IMetaData* CValueForm::GetMetaData() const
{
	if (m_sourceObject != nullptr) {
		const IValueMetaObject* metaObject = m_sourceObject->GetSourceMetaObject();
		if (metaObject != nullptr)
			return metaObject->GetMetaData();
	}

	return m_metaFormObject != nullptr ?
		m_metaFormObject->GetMetaData() :
		nullptr;
}

form_identifier_t CValueForm::GetTypeForm() const
{
	return m_metaFormObject != nullptr ?
		m_metaFormObject->GetTypeForm() :
		m_formType;
}

bool CValueForm::IsEditable() const
{
	if (m_sourceObject != nullptr) {
		const IValueMetaObject* metaObject = m_sourceObject->GetSourceMetaObject();
		if (metaObject != nullptr)
			return metaObject->IsEditable();
	}

	return m_metaFormObject != nullptr ?
		m_metaFormObject->IsEditable() :
		true;
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

enum Prop {
	eThisForm = 0,
	eControls,
	eDataSource,
	eModified,
	eFormOwner,
	eUniqueKey,
	eCloseOnChoice,
	eCloseOnOwnerClose
};

enum Func
{
	enShow = 0,
	enActivate,
	enUpdate,
	enClose,
	enIsShown,
	enAttachIdleHandler,
	enDetachIdleHandler,

	enNotifyChoice,
};

void CValueForm::PrepareNames() const
{
	//default element
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendProp(thisForm, true, false, eThisForm, eSystem);
	m_methodHelper->AppendProp(wxT("controls"), true, false, eControls, eSystem);
	m_methodHelper->AppendProp(wxT("dataSource"), true, false, eDataSource, eSystem);
	m_methodHelper->AppendProp(wxT("modified"), eModified, eSystem);
	m_methodHelper->AppendProp(wxT("formOwner"), eFormOwner, eSystem);
	m_methodHelper->AppendProp(wxT("uniqueKey"), eUniqueKey, eSystem);

	m_methodHelper->AppendProp(wxT("closeOnChoice"), eCloseOnChoice, eSystem);
	m_methodHelper->AppendProp(wxT("closeOnOwnerClose"), eCloseOnOwnerClose, eSystem);

	m_methodHelper->AppendProc(wxT("show"), "show()");
	m_methodHelper->AppendProc(wxT("activate"), "activate()");
	m_methodHelper->AppendProc(wxT("update"), "update()");
	m_methodHelper->AppendProc(wxT("close"), "close()");
	m_methodHelper->AppendFunc(wxT("isShown"), "isShown()");
	m_methodHelper->AppendProc(wxT("attachIdleHandler"), 3, "attachIdleHandler(procedureName, interval, single)");
	m_methodHelper->AppendProc(wxT("detachIdleHandler"), 1, "detachIdleHandler(procedureName)");
	m_methodHelper->AppendProc(wxT("notifyChoice"), 1, "notifyChoice(value)");

	//from property 
	for (unsigned int idx = 0; idx < IPropertyObject::GetPropertyCount(); idx++) {
		IProperty* property = IPropertyObject::GetProperty(idx);
		if (property == nullptr)
			continue;
		m_methodHelper->AppendProp(property->GetName(), idx, eProperty);
	}

	if (m_procUnit != nullptr) {
		CByteCode* byteCode = m_procUnit->GetByteCode();
		if (byteCode != nullptr) {
			for (auto exportVariable : byteCode->m_listExportVar)
				m_methodHelper->AppendProp(
					exportVariable.first,
					exportVariable.second,
					eProcUnit
				);
			for (auto exportFunction : byteCode->m_listExportFunc)
				m_methodHelper->AppendMethod(
					exportFunction.first,
					byteCode->GetNParams(exportFunction.second),
					byteCode->HasRetVal(exportFunction.second),
					exportFunction.second,
					eProcUnit
				);
		}
	}

	for (auto& control : m_listControl) {
		if (!control->HasValueInControl())
			continue;

		m_methodHelper->AppendProp(
			control->GetControlName(),
			control->GetControlID(),
			eAttribute
		);
	}

	m_formCollectionControl->PrepareNames();
}

bool CValueForm::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProcUnit) {
		if (m_procUnit != nullptr) {
			return m_procUnit->SetPropVal(GetPropName(lPropNum), varPropVal);
		}
	}
	else if (lPropAlias == eProperty) {
		return IValueFrame::SetPropVal(lPropNum, varPropVal);
	}
	else if (lPropAlias == eSystem) {
		switch (m_methodHelper->GetPropData(lPropNum)) {
		case eModified:
			Modify(varPropVal.GetBoolean());
			return true;
		case eCloseOnChoice:
			m_closeOnChoice = varPropVal.GetBoolean();
			return true;
		case eCloseOnOwnerClose:
			m_closeOnOwnerClose = varPropVal.GetBoolean();
			return true;
		}
	}
	else if (lPropAlias == eAttribute) {
		unsigned int id = m_methodHelper->GetPropData(lPropNum);
		auto& it = std::find_if(m_listControl.begin(), m_listControl.end(),
			[id](const IValueFrame* control) {
				return id == control->GetControlID();
			}
		);
		if (it != m_listControl.end())
			return (*it)->SetControlValue(varPropVal);
	}
	return false;
}

bool CValueForm::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProcUnit) {
		if (m_procUnit != nullptr) {
			return m_procUnit->GetPropVal(
				GetPropName(lPropNum), pvarPropVal
			);
		}
	}
	else if (lPropAlias == eProperty) {
		return IValueFrame::GetPropVal(lPropNum, pvarPropVal);
	}
	else if (lPropAlias == eSystem) {
		switch (m_methodHelper->GetPropData(lPropNum))
		{
		case eThisForm:
			pvarPropVal = GetValue();
			return true;
		case eControls:
			pvarPropVal = m_formCollectionControl;
			return true;
		case eDataSource:
			pvarPropVal = dynamic_cast<CValue*>(m_sourceObject);
			return true;
		case eModified:
			pvarPropVal = IsModified();
			return true;
		case eFormOwner:
			pvarPropVal = dynamic_cast<CValue*>(m_controlOwner);
			return true;
		case eUniqueKey:
			pvarPropVal = CValue::CreateAndPrepareValueRef<CValueGuid>(m_formKey);
			return true;
		case eCloseOnChoice:
			pvarPropVal = m_closeOnChoice;
			return true;
		case eCloseOnOwnerClose:
			pvarPropVal = m_closeOnOwnerClose;
			return true;
		}
	}
	else if (lPropAlias == eAttribute) {
		unsigned int id = m_methodHelper->GetPropData(lPropNum);
		auto& it = std::find_if(m_listControl.begin(), m_listControl.end(),
			[id](const IValueFrame* control) {
				return id == control->GetControlID();
			}
		);
		if (it != m_listControl.end())
			return (*it)->GetControlValue(pvarPropVal);
	}

	return false;
}

bool CValueForm::CallAsProc(const long lMethodNum, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enShow: ShowForm();
		return true;
	case enActivate: ActivateForm();
		return true;
	case enUpdate: UpdateForm();
		return true;
	case enAttachIdleHandler: AttachIdleHandler(paParams[0]->GetString(), paParams[1]->GetInteger(), paParams[2]->GetBoolean());
		return true;
	case enDetachIdleHandler: DetachIdleHandler(paParams[0]->GetString());
		return true;
	case enNotifyChoice:
		NotifyChoice(*paParams[0]);
		return true;
	}

	return IModuleDataObject::ExecuteProc(
		GetMethodName(lMethodNum), paParams, lSizeArray
	);
}

bool CValueForm::CallAsFunc(const long lMethodNum, CValue& pvarRetValue, CValue** paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case enClose:
		pvarRetValue = CloseForm();
		return true;
	case enIsShown:
		pvarRetValue = IsShown();
		return true;
	}

	return IModuleDataObject::ExecuteFunc(
		GetMethodName(lMethodNum), pvarRetValue, paParams, lSizeArray
	);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

S_CONTROL_TYPE_REGISTER(CValueForm, "clientForm", "form", g_controlFormCLSID);