/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file flowmapper.cpp Definition of flowmapper. */

#include "../stdafx.h"
#include "flowmapper.h"

/**
 * Map the paths generated by the MCF solver into flows associated with nodes.
 * @param component the link graph component to be used.
 */
void FlowMapper::Run(LinkGraphJob &job) const
{
	for (NodeID node_id = 0; node_id < job.Size(); ++node_id) {
		Node prev_node = job[node_id];
		StationID prev = prev_node.Station();
		PathList &paths = prev_node.Paths();
		for (PathList::iterator i = paths.begin(); i != paths.end(); ++i) {
			Path *path = *i;
			uint flow = path->GetFlow();
			if (flow == 0) break;
			Node node = job[path->GetNode()];
			StationID via = node.Station();
			StationID origin = job[path->GetOrigin()].Station();
			assert(prev != via && via != origin);
			/* Mark all of the flow for local consumption at "first". */
			node.Flows().AddFlow(origin, via, flow);
			if (prev != origin) {
				/* Pass some of the flow marked for local consumption at "prev" on
				 * to this node. */
				prev_node.Flows().PassOnFlow(origin, via, flow);
			} else {
				/* Prev node is origin. Simply add flow. */
				prev_node.Flows().AddFlow(origin, via, flow);
			}
		}
	}

	for (NodeID node_id = 0; node_id < job.Size(); ++node_id) {
		/* Remove local consumption shares marked as invalid. */
		Node node = job[node_id];
		FlowStatMap &flows = node.Flows();
		flows.FinalizeLocalConsumption(node.Station());
		if (this->scale) {
			/* Scale by time the graph has been running without being compressed. Add 1 to avoid
			 * division by 0 if spawn date == last compression date. This matches
			 * LinkGraph::Monthly(). */
			uint runtime = job.JoinDate() - job.Settings().recalc_time - job.LastCompression() + 1;
			for (FlowStatMap::iterator i = flows.begin(); i != flows.end(); ++i) {
				i->second.ScaleToMonthly(runtime);
			}
		}
		/* Clear paths. */
		PathList &paths = node.Paths();
		for (PathList::iterator i = paths.begin(); i != paths.end(); ++i) {
			delete *i;
		}
		paths.clear();
	}
}
