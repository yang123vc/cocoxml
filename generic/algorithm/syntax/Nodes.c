/*-------------------------------------------------------------------------
  Author (C) 2008, Charles Wang <charlesw123456@gmail.com>

  This program is free software; you can redistribute it and/or modify it 
  under the terms of the GNU General Public License as published by the 
  Free Software Foundation; either version 2, or (at your option) any 
  later version.

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
  for more details.

  You should have received a copy of the GNU General Public License along 
  with this program; if not, write to the Free Software Foundation, Inc., 
  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

  As an exception, it is allowed to write an extension of Coco/R that is
  used as a plugin in non-free software.

  If not otherwise stated, any source code generated by Coco/R (other than 
  Coco/R itself) does not fall under the GNU General Public License.
-------------------------------------------------------------------------*/
#include  "syntax/Nodes.h"

static void CcNodeT_Destruct(CcObject_t * self)
{
}
static const CcNodeType_t NodeT = {
    { sizeof(CcNodeT_t), "node_t", CcNodeT_Destruct }
};
const CcNodeType_t * node_t = &NodeT;

static void CcNodePR_Destruct(CcObject_t * self)
{
}
static const CcNodeType_t NodePR = {
    { sizeof(CcNodePR_t), "node_pr", CcNodePR_Destruct }
};
const CcNodeType_t * node_pr = &NodePR;

static void CcNodeNT_Destruct(CcObject_t * self)
{
}
static const CcNodeType_t NodeNT = {
    { sizeof(CcNodeNT_t), "node_nt", CcNodeNT_Destruct }
};
const CcNodeType_t * node_nt = &NodeNT;

static void CcNodeANY_Destruct(CcObject_t * self)
{
}
static const CcNodeType_t NodeANY = {
    { sizeof(CcNodeANY_t), "node_any", CcNodeANY_Destruct }
};
const CcNodeType_t * node_any = &NodeANY;

static void CcNodeEPS_Destruct(CcObject_t * self)
{
}
static const CcNodeType_t NodeEPS = {
    { sizeof(CcNodeEPS_t), "node_eps", CcNodeEPS_Destruct }
};
const CcNodeType_t * node_eps = &NodeEPS;

static void CcNodeSYNC_Destruct(CcObject_t * self)
{
}
static const CcNodeType_t NodeSYNC = {
    { sizeof(CcNodeSYNC_t), "node_sync", CcNodeSYNC_Destruct }
};
const CcNodeType_t * node_sync = &NodeSYNC;

static void CcNodeSEM_Destruct(CcObject_t * self)
{
}
static const CcNodeType_t NodeSEM = {
    { sizeof(CcNodeSEM_t), "node_sem", CcNodeSEM_Destruct }
};
const CcNodeType_t * node_sem = &NodeSEM;

static void CcNodeRSLV_Destruct(CcObject_t * self)
{
}
static const CcNodeType_t NodeRSLV = {
    { sizeof(CcNodeSEM_t), "node_rslv", CcNodeRSLV_Destruct }
};
const CcNodeType_t * node_rslv = &NodeRSLV;