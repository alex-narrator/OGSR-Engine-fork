#include "stdafx.h"
#include "phrasedialog.h"
#include "phrasedialogmanager.h"
#include "gameobject.h"
#include "ai_debug.h"

SPhraseDialogData::SPhraseDialogData()
{
    m_PhraseGraph.clear();
    m_iPriority = 0;
}

SPhraseDialogData::~SPhraseDialogData() {}

CPhraseDialog::CPhraseDialog()
{
    m_bFinished = false;
    m_pSpeakerFirst = NULL;
    m_pSpeakerSecond = NULL;
    m_DialogId = NULL;
}

CPhraseDialog::~CPhraseDialog() {}

void CPhraseDialog::Init(CPhraseDialogManager* speaker_first, CPhraseDialogManager* speaker_second)
{
    THROW(!IsInited());

    m_pSpeakerFirst = speaker_first;
    m_pSpeakerSecond = speaker_second;

    m_SaidPhraseID = "";
    m_PhraseVector.clear();

    CPhraseGraph::CVertex* phrase_vertex = data()->m_PhraseGraph.vertex("0");
    THROW(phrase_vertex);
    m_PhraseVector.push_back(phrase_vertex->data());

    m_bFinished = false;
    m_bFirstIsSpeaking = true;
}

//обнуляем все связи
void CPhraseDialog::Reset() {}

CPhraseDialogManager* CPhraseDialog::OurPartner(CPhraseDialogManager* dialog_manager) const
{
    if (FirstSpeaker() == dialog_manager)
        return SecondSpeaker();
    else
        return FirstSpeaker();
}

CPhraseDialogManager* CPhraseDialog::CurrentSpeaker() const { return FirstIsSpeaking() ? m_pSpeakerFirst : m_pSpeakerSecond; }
CPhraseDialogManager* CPhraseDialog::OtherSpeaker() const { return (!FirstIsSpeaking()) ? m_pSpeakerFirst : m_pSpeakerSecond; }

//предикат для сортировки вектора фраз
static bool PhraseGoodwillPred(const CPhrase* phrase1, const CPhrase* phrase2) { return phrase1->GoodwillLevel() > phrase2->GoodwillLevel(); }

bool CPhraseDialog::SayPhrase(DIALOG_SHARED_PTR& phrase_dialog, const shared_str& phrase_id)
{
    THROW(phrase_dialog->IsInited());

    phrase_dialog->m_SaidPhraseID = phrase_id;

    bool first_is_speaking = phrase_dialog->FirstIsSpeaking();
    phrase_dialog->m_bFirstIsSpeaking = !phrase_dialog->m_bFirstIsSpeaking;

    const CGameObject* pSpeakerGO1 = smart_cast<const CGameObject*>(phrase_dialog->FirstSpeaker());
    VERIFY(pSpeakerGO1);
    const CGameObject* pSpeakerGO2 = smart_cast<const CGameObject*>(phrase_dialog->SecondSpeaker());
    VERIFY(pSpeakerGO2);
    if (!first_is_speaking)
        std::swap(pSpeakerGO1, pSpeakerGO2);

    CPhraseGraph::CVertex* phrase_vertex = phrase_dialog->data()->m_PhraseGraph.vertex(phrase_dialog->m_SaidPhraseID);
    THROW(phrase_vertex);

    CPhrase* last_phrase = phrase_vertex->data();

    //вызвать скриптовую присоединенную функцию
    //активируется после сказанной фразы
    //первый параметр - тот кто говорит фразу, второй - тот кто слушает
    last_phrase->m_PhraseScript.Action(pSpeakerGO1, pSpeakerGO2, *phrase_dialog->m_DialogId, phrase_id.c_str());

    //больше нет фраз, чтоб говорить
    phrase_dialog->m_PhraseVector.clear();
    if (phrase_vertex->edges().empty())
    {
        phrase_dialog->m_bFinished = true;
    }
    else
    {
        //обновить список фраз, которые сейчас сможет говорить собеседник
        for (xr_vector<CPhraseGraph::CEdge>::const_iterator it = phrase_vertex->edges().begin(); it != phrase_vertex->edges().end(); ++it)
        {
            const CPhraseGraph::CEdge& edge = *it;
            CPhraseGraph::CVertex* next_phrase_vertex = phrase_dialog->data()->m_PhraseGraph.vertex(edge.vertex_id());
            THROW(next_phrase_vertex);
            shared_str next_phrase_id = next_phrase_vertex->vertex_id();
            if (next_phrase_vertex->data()->m_PhraseScript.Precondition(pSpeakerGO2, pSpeakerGO1, *phrase_dialog->m_DialogId, phrase_id.c_str(), next_phrase_id.c_str()))
            {
                phrase_dialog->m_PhraseVector.push_back(next_phrase_vertex->data());
#ifdef DEBUG
                if (psAI_Flags.test(aiDialogs))
                {
                    LPCSTR phrase_text = next_phrase_vertex->data()->GetText();
                    shared_str id = next_phrase_vertex->data()->GetID();
                    Msg("----added phrase text [%s]phrase_id=[%s] id=[%s] to dialog [%s]", phrase_text, phrase_id.c_str(), id.c_str(), *phrase_dialog->m_DialogId.c_str());
                }
#endif
            }
        }

        R_ASSERT2(!phrase_dialog->m_PhraseVector.empty(), make_string("No available phrase to say, dialog[%s]", *phrase_dialog->m_DialogId));

        //упорядочить списко по убыванию благосклонности
        std::sort(phrase_dialog->m_PhraseVector.begin(), phrase_dialog->m_PhraseVector.end(), PhraseGoodwillPred);
    }

    //сообщить CDialogManager, что сказана фраза
    //и ожидается ответ
    if (first_is_speaking)
        phrase_dialog->SecondSpeaker()->ReceivePhrase(phrase_dialog);
    else
        phrase_dialog->FirstSpeaker()->ReceivePhrase(phrase_dialog);

    return phrase_dialog ? !phrase_dialog->m_bFinished : true;
}

CPhrase* CPhraseDialog::GetPhrase(const shared_str& phrase_id)
{
    CPhraseGraph::CVertex* phrase_vertex = data()->m_PhraseGraph.vertex(phrase_id);
    THROW(phrase_vertex);

    return phrase_vertex->data();
}

LPCSTR CPhraseDialog::GetPhraseText(const shared_str& phrase_id, bool current_speaking)
{
    CPhrase* ph = GetPhrase(phrase_id);

    CGameObject* pSpeakerGO1 = (current_speaking) ? smart_cast<CGameObject*>(FirstSpeaker()) : 0;
    CGameObject* pSpeakerGO2 = (current_speaking) ? smart_cast<CGameObject*>(SecondSpeaker()) : 0;

    return ph->m_PhraseScript.GetScriptText(ph->GetText(), pSpeakerGO1, pSpeakerGO2, m_DialogId.c_str(), phrase_id.c_str());
}

LPCSTR CPhraseDialog::DialogCaption() { return data()->m_sCaption.size() ? *data()->m_sCaption : GetPhraseText("0"); }

int CPhraseDialog::Priority() { return data()->m_iPriority; }

void CPhraseDialog::Load(shared_str dialog_id)
{
    m_DialogId = dialog_id;

    inherited_shared::load_shared(m_DialogId, NULL);

    if (GetForceReload())
    {
        if (data()->m_sInitFunction.size())
        {
            data()->m_PhraseGraph.clear();

            luabind::functor<void> lua_function;
            bool functor_exists = ai().script_engine().functor(data()->m_sInitFunction.c_str(), lua_function);
            ASSERT_FMT_DBG(functor_exists, "!![%s] Cannot find precondition [%s]", __FUNCTION__, data()->m_sInitFunction.c_str());
            if (functor_exists)
                lua_function(this);
        }
    }
}

#include "script_engine.h"
#include "ai_space.h"

void CPhraseDialog::load_shared(LPCSTR)
{
    const ITEM_DATA& item_data = *id_to_index::GetById(m_DialogId);

    CUIXml* pXML = item_data._xml;
    pXML->SetLocalRoot(pXML->GetRoot());

    // loading from XML
    XML_NODE* dialog_node = pXML->NavigateToNode(id_to_index::tag_name, item_data.pos_in_file);
    THROW3(dialog_node, "dialog id=", *item_data.id);

    pXML->SetLocalRoot(dialog_node);

    SetPriority(pXML->ReadAttribInt(dialog_node, "priority", 0));

    //заголовок
    SetCaption(pXML->Read(dialog_node, "caption", 0, NULL));

    SetForceReload(!!pXML->ReadAttribInt(dialog_node, "force_reload", 0));

    //предикаты начала диалога
    data()->m_PhraseScript.Load(pXML, dialog_node);

    //заполнить граф диалога фразами
    data()->m_PhraseGraph.clear();

    XML_NODE* phrase_list_node = pXML->NavigateToNode(dialog_node, "phrase_list", 0);
    if (NULL == phrase_list_node && !GetForceReload())
    {
        data()->m_sInitFunction = pXML->Read(dialog_node, "init_func", 0, "");

        luabind::functor<void> lua_function;
        bool functor_exists = ai().script_engine().functor(data()->m_sInitFunction.c_str(), lua_function);
        ASSERT_FMT_DBG(functor_exists, "!![%s] Cannot find precondition [%s]", __FUNCTION__, data()->m_sInitFunction.c_str());
        if (functor_exists)
            lua_function(this);

        return;
    }

    THROW3(pXML->GetNodesNum(phrase_list_node, "phrase"), "dialog %s has no phrases at all", *item_data.id);

    pXML->SetLocalRoot(phrase_list_node);

#ifdef DEBUG // debug & mixed
    LPCSTR wrong_phrase_id = pXML->CheckUniqueAttrib(phrase_list_node, "phrase", "id");
    THROW3(wrong_phrase_id == NULL, *item_data.id, wrong_phrase_id);
#endif

    //ищем стартовую фразу
    XML_NODE* phrase_node = pXML->NavigateToNodeWithAttribute("phrase", "id", "0");
    THROW(phrase_node);
    AddPhrase(pXML, phrase_node, "0", "");
}

void CPhraseDialog::SetCaption(LPCSTR str) { data()->m_sCaption = str; }

void CPhraseDialog::SetPriority(int val) { data()->m_iPriority = val; }

CPhrase* CPhraseDialog::AddPhrase(LPCSTR text, const shared_str& phrase_id, const shared_str& prev_phrase_id, int goodwil_level)
{
    CPhrase* phrase = NULL;
    CPhraseGraph::CVertex* _vertex = data()->m_PhraseGraph.vertex(phrase_id);
    if (!_vertex)
    {
        phrase = xr_new<CPhrase>();
        VERIFY(phrase);
        phrase->SetID(phrase_id);

        phrase->SetText(text);
        phrase->m_iGoodwillLevel = goodwil_level;

        data()->m_PhraseGraph.add_vertex(phrase, phrase_id);
    }

    if (prev_phrase_id != "")
        data()->m_PhraseGraph.add_edge(prev_phrase_id, phrase_id, 0.f);

    return phrase;
}

void CPhraseDialog::AddPhrase(CUIXml* pXml, XML_NODE* phrase_node, const shared_str& phrase_id, const shared_str& prev_phrase_id)
{
    LPCSTR sText = pXml->Read(phrase_node, "text", 0, "");
    int gw = pXml->ReadInt(phrase_node, "goodwill", 0, -10000);
    CPhrase* ph = AddPhrase(sText, phrase_id, prev_phrase_id, gw);
    if (!ph)
        return;

    ph->m_PhraseScript.Load(pXml, phrase_node);

    //фразы которые собеседник может говорить после этой
    int next_num = pXml->GetNodesNum(phrase_node, "next");
    for (int i = 0; i < next_num; ++i)
    {
        LPCSTR next_phrase_id_str = pXml->Read(phrase_node, "next", i, "");
        XML_NODE* next_phrase_node = pXml->NavigateToNodeWithAttribute("phrase", "id", next_phrase_id_str);
        ASSERT_FMT(next_phrase_node, "!!Can`t find next phrase with id: [%s]! Phrase text: [%s]. Phrase dialog [%s]", next_phrase_id_str, sText, m_DialogId.c_str());

        AddPhrase(pXml, next_phrase_node, next_phrase_id_str, phrase_id);
    }
}

bool CPhraseDialog::Precondition(const CGameObject* pSpeaker1, const CGameObject* pSpeaker2)
{
    return data()->m_PhraseScript.Precondition(pSpeaker1, pSpeaker2, m_DialogId.c_str(), "", "");
}

void CPhraseDialog::InitXmlIdToIndex()
{
    if (!id_to_index::tag_name)
        id_to_index::tag_name = "dialog";
    if (!id_to_index::file_str)
        id_to_index::file_str = pSettings->r_string("dialogs", "files");
}

bool CPhraseDialog::allIsDummy()
{
    PHRASE_VECTOR_IT it = m_PhraseVector.begin();
    bool bAllIsDummy = true;
    for (; it != m_PhraseVector.end(); ++it)
    {
        if (!(*it)->IsDummy())
            bAllIsDummy = false;
    }

    return bAllIsDummy;
}
