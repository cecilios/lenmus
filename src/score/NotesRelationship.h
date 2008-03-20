//--------------------------------------------------------------------------------------
//    LenMus Phonascus: The teacher of music
//    Copyright (c) 2002-2008 Cecilio Salmeron
//
//    This program is free software; you can redistribute it and/or modify it under the 
//    terms of the GNU General Public License as published by the Free Software Foundation;
//    either version 2 of the License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but WITHOUT ANY 
//    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
//    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along with this 
//    program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, 
//    Fifth Floor, Boston, MA  02110-1301, USA.
//
//    For any comment, suggestion or feature request, please contact the manager of 
//    the project at cecilios@users.sourceforge.net
//
//-------------------------------------------------------------------------------------

#ifndef __LM_NOTESRELATIONSHIP_H__        //to avoid nested includes
#define __LM_NOTESRELATIONSHIP_H__

//#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
//#pragma interface ".cpp"
//#endif

#include <list>
#include <algorithm>

class lmNoteRest;
class lmNote;
class lmUndoData;

//instead of using dynamic_cast I define a class type value
enum lmERelationshipClass
{
	lm_eBeamClass = 0,
	lm_eChordClass,
	lm_eTupletClass,
};

template <class T>      // T is either lmNote or lmNoteRest
class lmRelationship
{
public:
	virtual ~lmRelationship() {}

    virtual void Include(T* pNR, int nIndex = -1)=0;
    virtual void Remove(T* pNR)=0;
    virtual T* GetStartNoteRest()=0;
    virtual T* GetEndNoteRest()=0;
    virtual void Save(lmUndoData* pUndoData)=0;
	inline lmERelationshipClass GetClass() { return m_nClass; }


protected:
	lmRelationship(lmERelationshipClass nClass) { m_nClass = nClass; }

	lmERelationshipClass	m_nClass;

};


template <class T>      // T is either lmNote or lmNoteRest
class lmBinaryRelationship : public lmRelationship<T>
{
public:
	virtual ~lmBinaryRelationship() {};

	//implementation of lmRelationship virtual methods
    virtual void Include(T* pNR, int nIndex = -1)=0;
    virtual void Remove(T* pNR)=0;
    inline T* GetStartNoteRest() { return m_pStartNote; }
    inline T* GetEndNoteRest() { return m_pEndNote; }
    virtual void Save(lmUndoData* pUndoData)=0;


protected:
    lmBinaryRelationship(lmERelationshipClass nClass, T* pStartNote, T* pEndNote)
		: lmRelationship(nClass) {}


    T*		m_pStartNote;        //notes related by this object
    T*		m_pEndNote;

};

template <class T>      // T is either lmNote or lmNoteRest
class lmMultipleRelationship : public lmRelationship<T>
{
public:
    virtual ~lmMultipleRelationship();
    
	//implementation of lmRelationship virtual methods
    virtual void Include(T* pNR, int nIndex = -1);
    virtual void Remove(T* pNR);
    inline int NumNotes() { return (int)m_Notes.size(); }
    inline T* GetStartNoteRest() { return m_Notes.front(); }
    inline T* GetEndNoteRest() { return m_Notes.back(); }
    virtual void Save(lmUndoData* pUndoData)=0;
    
    //specific methods
        
    virtual int GetNoteIndex(T* pNR);

	wxString Dump();


protected:
    lmMultipleRelationship(lmERelationshipClass nClass);
    lmMultipleRelationship(lmERelationshipClass nClass, T* pNote);
	lmMultipleRelationship(lmERelationshipClass nClass, T* pFirstNote, lmUndoData* pUndoData);

	void RemoveAllNotes();


    std::list<T*>   m_Notes;        //list of note/rests that form the relation   

};

//--------------------------------------------------------------------------------------------
// lmMultipleRelationship implementation
//--------------------------------------------------------------------------------------------

template <class T>
lmMultipleRelationship<T>::lmMultipleRelationship(lmERelationshipClass nClass)
	: lmRelationship<T>(nClass)
{
}

template <class T>
lmMultipleRelationship<T>::lmMultipleRelationship(lmERelationshipClass nClass, T* pNote)
	: lmRelationship<T>(nClass)
{
    Include(pNote);
}

template <class T>
lmMultipleRelationship<T>::lmMultipleRelationship(lmERelationshipClass nClass,
												  T* pFirstNote, lmUndoData* pUndoData)
	: lmRelationship<T>(nClass)
{
}

template <class T>
lmMultipleRelationship<T>::~lmMultipleRelationship()
{
    //AWARE: notes must not be deleted when deleting the list, as they are part of a lmScore
    //and will be deleted there.
    RemoveAllNotes();
}

template <class T>
void lmMultipleRelationship<T>::Include(T* pNR, int nIndex)
{
    // Add a note to the relation. Index is the position that the added note/rest must occupy
	// (0..n). If -1, note/rest will be added at the end.

	//add the note/rest
	if (nIndex == -1 || nIndex == NumNotes())
		m_Notes.push_back(pNR);
	else
	{
		int iN;
		std::list<T*>::iterator it;
		for(iN=0, it=m_Notes.begin(); it != m_Notes.end(); ++it, iN++)
		{
			if (iN == nIndex)
			{
				//insert before current item
				m_Notes.insert(it, pNR);
				break;
			}
		}
	}
	wxLogMessage(Dump());
	pNR->OnIncludedInRelationship((void*)this, GetClass());
}

template <class T>
wxString lmMultipleRelationship<T>::Dump()
{
	wxString sDump = _T("");
	std::list<T*>::iterator it;
	for(it=m_Notes.begin(); it != m_Notes.end(); ++it)
	{
		sDump += wxString::Format(_T("Note id = %d\n"), (*it)->GetID());
	}
	return sDump;
}

template <class T>
int lmMultipleRelationship<T>::GetNoteIndex(T* pNR)
{
	//returns the position in the notes list (0..n)

	wxASSERT(NumNotes() > 0);

	int iN;
    std::list<T*>::iterator it;
    for(iN=0, it=m_Notes.begin(); it != m_Notes.end(); ++it, iN++)
	{
		if (pNR == *it) return iN;
	}
    wxASSERT(false);	//note not found
	return 0;			//compiler happy
}

template <class T>
void lmMultipleRelationship<T>::RemoveAllNotes()
{
    //the relationship is going to be removed. Release all notes

    //ask each note to remove relationship information
    std::list<T*>::iterator it;
    for(it=m_Notes.begin(); it != m_Notes.end(); ++it)
	{
        (*it)->OnRemovedFromRelationship((void*)this, GetClass());
	}

    //remove all notes from beam
    m_Notes.clear();
}

template <class T>
void lmMultipleRelationship<T>::Remove(T* pNR)
{
    //remove note/rest

    wxASSERT(NumNotes() > 0);

    std::list<T*>::iterator it;
    it = std::find(m_Notes.begin(), m_Notes.end(), pNR);
    m_Notes.erase(it);
	pNR->OnRemovedFromRelationship((void*)this, GetClass());
}



#endif    // __LM_NOTESRELATIONSHIP_H__

