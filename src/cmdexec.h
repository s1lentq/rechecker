/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#pragma once

class CExecMngr
{
public:
	void Add(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash);
	void ExecuteCommand(IGameClient *pClient);
	void Clear(IGameClient *pClient = NULL);

private:
	class CBufExec
	{
	public:
		CBufExec(IGameClient *pClient, CResourceBuffer *pResource, uint32 responseHash);
		~CBufExec();

		int GetUserID() const { return m_UserID; };
		IGameClient *GetGameClient() const { return m_pClient; };
		CResourceBuffer *GetResource() const { return m_pResource; };
		uint32 GetClientHash() const { return m_ClientHash; };

	private:
		int m_UserID;
		IGameClient *m_pClient;
		CResourceBuffer *m_pResource;
		uint32 m_ClientHash;
	};

	typedef std::vector<CBufExec *> CBufExecList;
	CBufExecList m_execList;
};

extern CExecMngr Exec;
extern void StringReplace(char *src, const char *strold, const char *strnew);

