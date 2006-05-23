using System;
using System.Collections;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Windows.Forms;
using System.Xml;
using TriggerEdit.Definitions;

namespace TriggerEdit
{
	public class TriggerContainer
	{
		//-------------
		// nested types
		//-------------

		#region

		public struct Link
		{
			//-------------
			// nested types
			//-------------

			#region

			[ Flags ]
			public enum Status
			{
				None     = 0x00,
				Selected = 0x01,
				Bright   = 0x02,
				Dim      = 0x04
			}

			#endregion

			//----------
			// interface
			//----------

			#region

			public Link(int tail, int head)
			{
				head_       = head;
				tail_       = tail;
				status_     = Status.None;
				group_      = 0;
				persistent_ = false;
			}

			public Link(int tail, int head, int group, bool is_thin)
			{
				head_       = head;
				tail_       = tail;
				status_     = Status.None;
				group_      = group;
				persistent_ = !is_thin;
			}

			public Color Fill
			{
				get
				{
					if (0 != (status_ & Status.Selected))
						return Color.Yellow;
					if (0 != (status_ & Status.Bright))
						return Color.Yellow;
					if (0 != (status_ & Status.Dim))
						return Color.Gold;
					return Color.Red;
				}
			}

			public Color HeadFill
			{
				get
				{
					if (0 != (status_ & Status.Bright))
						return Color.Yellow;
					if (0 != (status_ & Status.Dim))
						return Color.Gold;
					return Link.colors_[group_];
				}
			}

			#endregion

			//-----
			// data
			//-----

			#region

			public int    head_;
			public int    tail_;
			public Status status_;
			public int    group_;
			public bool   persistent_;

			#endregion

			#region static

			// 10 colours - one for each group
			public static Color[] colors_ = {
														  Color.Red,
														  Color.LightBlue,
														  Color.LightGreen,
														  Color.Khaki,
														  Color.Plum,
														  Color.YellowGreen,
														  Color.Orange,
														  Color.Gray,
														  Color.DeepPink,
														  Color.Tan
													  };

			public static string[] color_strings_ = {
																	 "STRATEGY_RED",
																	 "STRATEGY_GREEN",
																	 "STRATEGY_BLUE",
																	 "STRATEGY_YELLOW",
																	 "STRATEGY_COLOR_0",
																	 "STRATEGY_COLOR_1",
																	 "STRATEGY_COLOR_2",
																	 "STRATEGY_COLOR_3",
																	 "STRATEGY_COLOR_4",
																	 "STRATEGY_COLOR_5"
															  };

			#endregion

			}

		public struct GhostLink
		{
			public GhostLink(PointF tail, PointF head)
			{
				head_      = head;
				tail_      = tail;
				snap_head_ = true;
				snap_tail_ = true;
			}
			public PointF head_;
			public PointF tail_;
			public bool   snap_head_;
			public bool   snap_tail_;
		}

		public struct TriggerDescription
		{
			#region nested types

			public enum State
			{
				Checking,
				Sleeping,
				Done
			}

			[ Flags ]
			public enum Status
			{
				None     = 0x00,
				Bright   = 0x01,
				Dim      = 0x02,
				Ghost    = 0x04,
				Selected = 0x08,
				All      = 0xFF
			}

			#endregion

			#region interface

			/// <summary>
			/// Input from XML.
			/// </summary>
			/// <param name="node">Root XML node for the trigger.</param>
			/// <param name="state">State of the trigger.</param>
			public void Serialize(XmlNode node, out State state, out PointF position)
			{
				// read name
				name_ = node.SelectSingleNode(
					"string[@name=\"name\"]").InnerText.Trim();
				// read state
				XmlNode state_node = node.SelectSingleNode(
					"value[@name=\"state_\"]");
				state = State.Sleeping;
				switch (state_node.InnerText.Trim())
				{
					case "CHECKING": state = State.Checking; break;
					case "SLEEPING": state = State.Sleeping; break;
					case "DONE":     state = State.Done;     break;
				}
				// read action
				XmlNode action_node = node.SelectSingleNode("set[@name=\"action\"]");
				if (null != action_node)
					action_ = Action.CreateInstance(action_node);
				// read condition
				XmlNode condition_node = node.SelectSingleNode("set[@name=\"condition\"]");
				if (null != condition_node)
					condition_ = Condition.CreateInstance(condition_node);
				// read position
				position = PointF.Empty;
				try
				{
					position.X = float.Parse(node.SelectSingleNode(
						"set[@name=\"position\"]/float[@name=\"x\"]").InnerText.Trim());
					position.Y = float.Parse(node.SelectSingleNode(
						"set[@name=\"position\"]/float[@name=\"y\"]").InnerText.Trim());
				}
				catch {}
			}

			public void Serialize(
				ScriptXmlWriter w,
				IList           links,
				IList     descriptions,
				State           state,
				PointF          position)
			{
				// start trigger
				w.WriteStartElement("set");
				w.WriteElement("string", "name", name_);
				// condition
				if (condition_ != null)
					condition_.Serialize(w);
				else
					w.WriteElement("set", "condition", "");
				// action
				if (action_ != null)
					action_.Serialize(w);
				else
					w.WriteElement("int", "action", "0");
				// outgoing links
				w.WriteStartNamedElement("array", "outcomingLinks");
				foreach (Link link in links)
				{
					// get name
					string name  = ((TriggerDescription)descriptions[link.head_]).name_;
					string color = Link.color_strings_[link.group_];
					string type  = link.persistent_ ? "THICK" : "THIN";
					w.WriteStartElement("set");
					w.WriteElement("string", "triggerName",    name);
					w.WriteElement("value",  "color",          color);
					w.WriteElement("value",  "type",           type);
					w.WriteElement("value",  "active_",        "false");
					w.WriteElement("int",    "parentOffsetX_", "0");
					w.WriteElement("int",    "parentOffsetY_", "0");
					w.WriteElement("int",    "childOffsetX_",  "0");
					w.WriteElement("int",    "childOffsetY_",  "0");
					w.WritePoint("parentOffset", Point.Empty);
					w.WritePoint("childOffset", Point.Empty);
					w.WriteEndElement();
				}
				w.WriteEndElement();
				// state
				switch (state)
				{
					case State.Checking: w.WriteElement("value", "state_", "CHECKING"); break;
					case State.Sleeping: w.WriteElement("value", "state_", "SLEEPING"); break;
					case State.Done:     w.WriteElement("value", "state_", "DONE");     break;
				}
				w.WriteElement("int", "executionCounter_", "0");
				w.WriteElement("int", "internalColor_",    "0");
				w.WriteElement("int", "cellNumberX",       "0");
				w.WriteElement("int", "cellNumberY",       "0");
				w.WriteElement("int", "left_",             "0");
				w.WriteElement("int", "top_",              "0");
				w.WriteElement("int", "right_",            "0");
				w.WriteElement("int", "bottom_",           "0");
				w.WriteElement("int", "internalColor_",    "0");
				w.WritePoint("cellIndex", Point.Empty);
				w.WriteRect("boundingRect", Rectangle.Empty);
				// write the position
				w.WriteStartNamedElement("set", "position");
				w.WriteAttributeString("usage", "private");
				w.WriteElement("float", "x", position.X.ToString());
				w.WriteElement("float", "y", position.Y.ToString());
				w.WriteEndElement();
				// end trigger
				w.WriteEndElement();
			}

			public Color Fill
			{
				get
				{
					if (0 != (status_ & Status.Selected))
						return Color.Yellow;
					if (0 != (status_ & Status.Bright))
						return Color.Yellow;
					if (0 != (status_ & Status.Dim))
						return Color.Gold;
					return Color.Orange;
				}
			}

			public Color Outline
			{
				get
				{
					if (0 != (status_ & Status.Selected))
						return Color.Orange;
					return Fill;
				}
			}

			public float Opacity
			{
				get
				{
					if (0 != (status_ & Status.Ghost))
						return 0.5f;
					return 1.0f;
				}
			}

			#endregion

			#region data

			public Action    action_;
			public Condition condition_;
			public Group     group_;
			public string    name_;
			public Status    status_;
			public bool      is_virtual_;

			#endregion
		}

		public struct PositionHistory
		{
			public void Add(PointF point)
			{
				if (Norm(point.X - point2_.X, point.Y - point2_.Y) < 0.25)
				{
					point.X = (point2_.X + point3_.X) / 2;
					point.Y = (point2_.Y + point3_.Y) / 2;
				}
				point1_ = point2_;
				point2_ = point3_;
				point3_ = point;
			}
			public void Set(PointF point)
			{
				point1_ = point;
				point2_ = point;
				point3_ = point;
			}
			private float Norm(float x, float y)
			{
				return x * x + y * y;
			}
			// least recent
			public PointF point1_;
			public PointF point2_;
			public PointF point3_;
			// most recent
		}

		public class Group
		{
			public string comment_;
		}

		#endregion

		//----------
		// interface
		//----------

		#region

		public TriggerContainer()
		{
			g_marker_descriptions_ = new ArrayList();
			g_marker_positions_    = new ArrayList();
			g_links_               = new ArrayList();
			groups_                = new ArrayList();
		}

		public void Collapse(int index)
		{
			// count connections
			int in_count  = 0;
			int out_count = 0;
			for (int j = 0; j != count_; ++j)
			{
				if (adjacency_matrix_[index, j])
					++in_count;
				if (adjacency_matrix_[j, index])
					++out_count;
			}
			// get connections
			int[] in_indices  = new int[in_count];
			int[] out_indices = new int[out_count];
			int   in_iter   = 0;
			int   out_iter  = 0;
			for (int j = 0; j != count_; ++j)
			{
				if (adjacency_matrix_[index, j])
					in_indices[in_iter++] = j;
				if (adjacency_matrix_[j, index])
					out_indices[out_iter++] = j;
			}
			// create new links connecting all incoming nodes with every outgoing
			for (int in_i = 0; in_i != in_indices.Length; ++in_i)
				for (int out_i = 0; out_i != out_indices.Length; ++out_i)
					adjacency_list_.Add(new Link(out_indices[out_i], in_indices[in_i]));
			// remove the current trigger from the links list
			adjacency_list_.RemoveVertex(index);
			for (int i = 0; i != adjacency_list_.Count; ++i)
			{
				Link link = adjacency_list_[i];
				if (link.head_ > index)
					--link.head_;
				if (link.tail_ > index)
					--link.tail_;
				adjacency_list_[i] = link;
			}
			// adjust the rest of the data
			adjacency_matrix_.Collapse(index);
			DeleteDescriptionEntry    (index);
			DeleteHistoryEntry        (index);
			DeletePositionEntry       (index);
			DeleteVelocityEntry       (index);
			--count_;
		}

		public void AddNew()
		{
			adjacency_matrix_.Grow(1);
			CreateDescriptionEntry();
			CreateHistoryEntry();
			CreatePositionEntry();
			CreateVelocityEntry();
			++count_;
		}

		public void Delete(int index)
		{
			adjacency_list_.RemoveVertex(index);
			adjacency_matrix_.Delete    (index);
			DeleteDescriptionEntry      (index);
			DeleteHistoryEntry          (index);
			DeletePositionEntry         (index);
			DeleteVelocityEntry         (index);
			--count_;
		}

		public void Duplicate(int index)
		{
			DuplicateListEntry         (index);
			adjacency_matrix_.Duplicate(index);
			DuplicateDescriptionEntry  (index);
			DuplicateHistoryEntry      (index);
			DuplicatePositionEntry     (index);
			DuplicateVelocityEntrty    (index);
			++count_;
		}

		public void AddLink(int tail, int head)
		{
			AddLink(tail, head, 0, true);
		}

		public void AddLink(int tail, int head, int group, bool is_thin)
		{
			if (adjacency_matrix_[tail, head])
				throw new Exception("Link already exists.");
			adjacency_list_.Add(new Link(tail, head, group, is_thin));
			adjacency_matrix_[tail, head] = true;
		}

		public void RemoveLink(int tail, int head)
		{
			if (!adjacency_matrix_[tail, head])
				throw new Exception("Link does not exist.");
			adjacency_list_.Remove(tail, head);
			adjacency_matrix_[tail, head] = false;
		}

		/// <summary>
		/// Get index of the trigger with marker under the given point.
		/// </summary>
		public int GetIndex(PointF point, MarkerLayout layout)
		{
			float radius_h = layout.Width  / 2.0f;
			float radius_v = layout.Height / 2.0f;
			int index;
			for (index = 0; index != positions_.Length; ++index)
			{
				float x = positions_[index].X;
				float y = positions_[index].Y;
				if (
					x - radius_h <= point.X &&
					x + radius_h >= point.X &&
					y - radius_v <= point.Y &&
					y + radius_v >= point.Y)
					break;
			}
			return (positions_.Length == index) ? -1 : index;
		}

		/// <param name="position">in world coordinates</param>
		/// <returns></returns>
		public int GetLinkIndex(PointF position, float marker_width, float marker_height)
		{
			const int tolerance = 6;
			// find the link under the cursor
			// search unselected
			for (int i = 0; i != adjacency_list_.Count; ++i)
			{
				if (0 != (adjacency_list_[i].status_ & Link.Status.Selected))
					continue;
				// get head and tail of the link
				PointF t = GetInterpolatedPosition(adjacency_list_[i].tail_);
				PointF h = GetInterpolatedPosition(adjacency_list_[i].head_);
				SnapPoints(ref h, ref t, marker_width, marker_height);
				// check whether the point is within the bounding rectangle
				if (position.X < t.X - tolerance && position.X < h.X - tolerance)
					continue;
				if (position.X > t.X + tolerance && position.X > h.X + tolerance)
					continue;
				if (position.Y < t.Y - tolerance && position.Y < h.Y - tolerance)
					continue;
				if (position.Y > t.Y + tolerance && position.Y > h.Y + tolerance)
					continue;
				// translate the origin to the tail
				PointF l = new PointF(h.X - t.X,        h.Y - t.Y);
				PointF p = new PointF(position.X - t.X, position.Y - t.Y);
				// project the position vector onto the link vector
				float l_norm = (float)Math.Sqrt(l.X * l.X + l.Y * l.Y);
				float d = (l.X * p.X + l.Y * p.Y) / l_norm;
				// check length of the projection
				if (d < 0 || d > l_norm)
					continue;
				// get distance from the link
				PointF off = new PointF(p.X - l.X * d / l_norm, p.Y - l.Y * d / l_norm);
				d = off.X * off.X + off.Y * off.Y;
				// check the distance
				if (d > tolerance * tolerance)
					continue;
				return i;
			}
			// find the link under the cursor
			// search the rest
			for (int i = 0; i != adjacency_list_.Count; ++i)
			{
				// get head and tail of the link
				PointF t = GetInterpolatedPosition(adjacency_list_[i].tail_);
				PointF h = GetInterpolatedPosition(adjacency_list_[i].head_);
				SnapPoints(ref h, ref t, marker_width, marker_height);
				// check whether the point is within the bounding rectangle
				if (position.X < t.X - tolerance && position.X < h.X - tolerance)
					continue;
				if (position.X > t.X + tolerance && position.X > h.X + tolerance)
					continue;
				if (position.Y < t.Y - tolerance && position.Y < h.Y - tolerance)
					continue;
				if (position.Y > t.Y + tolerance && position.Y > h.Y + tolerance)
					continue;
				// translate the origin to the tail
				PointF l = new PointF(h.X - t.X,        h.Y - t.Y);
				PointF p = new PointF(position.X - t.X, position.Y - t.Y);
				// project the position vector onto the link vector
				float l_norm = (float)Math.Sqrt(l.X * l.X + l.Y * l.Y);
				float d = (l.X * p.X + l.Y * p.Y) / l_norm;
				// check length of the projection
				if (d < 0 || d > l_norm)
					continue;
				// get distance from the link
				PointF off = new PointF(p.X - l.X * d / l_norm, p.Y - l.Y * d / l_norm);
				d = off.X * off.X + off.Y * off.Y;
				// check the distance
				if (d > tolerance * tolerance)
					continue;
				return i;
			}
			return -1;
		}

		public void GetInterpolatedPosition(int index, out float x, out float y)
		{
			PositionHistory src = history_[index];
			x = (float)Math.Round(
				src.point1_.X * r0_ + src.point2_.X * r1_ + src.point3_.X * r2_);
			y = (float)Math.Round(
				src.point1_.Y * r0_ + src.point2_.Y * r1_ + src.point3_.Y * r2_);
		}

		public void GetInterpolatedPosition(int index, ref PointF point)
		{
			float x, y;
			GetInterpolatedPosition(index, out x, out y);
			point.X = x;
			point.Y = y;
		}

		public PointF GetInterpolatedPosition(int index)
		{
			PointF point = PointF.Empty;
			GetInterpolatedPosition(index, ref point);
			return point;
		}

		public void DeleteLink(int tail, int head)
		{
			if (tail == head)
				throw new ArgumentException("tail must not equal head");
			adjacency_list_.Remove(tail, head);
			adjacency_matrix_[tail, head] = false;
		}

		public float InterpolationRatio
		{
			get { return ratio_; }
			set
			{
				Debug.Assert(value <= 1.0f);
				r2_ =  value * value * 0.5f;                  // 1/2 * t^2
				r1_ = -value * value + value + 0.5f;          // -t^2 + t + 1/2
				r0_ = (1.0f - value) * (1.0f - value) * 0.5f; // 1/2 * (1 - t)^2
				ratio_ = value;
			}
		}

		public void Reset()
		{
			count_ = 2;
			// initialize state variables
			adjacency_list_   = new AdjacencyList();
			adjacency_matrix_ = new AdjacencyMatrix(count_);
			descriptions_     = new TriggerDescription[count_];
			history_          = new PositionHistory[count_];
			positions_        = new PointF[count_];
			velocities_       = new PointF[count_];
			// initialize the root node
			descriptions_[0].name_       = "[root]";
			descriptions_[0].is_virtual_ = true;
			// initialize the first node
			descriptions_[1].name_ = "[new]";
			AddLink(0, 1);
		}

		/// <summary>
		/// Input from XML.
		/// </summary>
		/// <param name="node">Script root.</param>
		/// <returns>Whether the serialization was successful.</returns>
		public bool Serialize(XmlNode node)
		{
			// extract data
			XmlNodeList trigger_nodes = node.SelectNodes(
				"//script"
				+ "/set[@name=\"TriggerChain\"]"
				+ "/array[@name=\"triggers\"]"
				+ "/set");
			count_ = trigger_nodes.Count + 1; // + the root trigger
			// initialize state variables
			adjacency_list_   = new AdjacencyList();
			adjacency_matrix_ = new AdjacencyMatrix(count_);
			descriptions_     = new TriggerDescription[count_];
			history_          = new PositionHistory[count_];
			positions_        = new PointF[count_];
			velocities_       = new PointF[count_];
			// creat the root node
			descriptions_[0].name_       = "[root]";
			descriptions_[0].is_virtual_ = true;
			try
			{
				XmlNode root_node = node.SelectSingleNode(
					"//script"
					+ "/set[@name=\"TriggerChain\"]"
					+ "/set[@name=\"[root]\"]"
					+ "/set[@name=\"position\"]");
				positions_[0].X = float.Parse(root_node.SelectSingleNode(
					"float[@name=\"x\"]").InnerText.Trim());
				positions_[0].Y = float.Parse(root_node.SelectSingleNode(
					"float[@name=\"y\"]").InnerText.Trim());
			}
			catch {}
			// serialize triggers and read in names
			Hashtable names = new Hashtable(count_);
			for (int i = 1; i != count_; ++i)
			{
				TriggerDescription.State state;
				descriptions_[i].Serialize(trigger_nodes[i - 1], out state, out positions_[i]);
				if (state == TriggerDescription.State.Checking)
					AddLink(0, i);
				if (!names.Contains(descriptions_[i].name_))
					names.Add(descriptions_[i].name_, i);
			}
			// add links
			for (int i = 1; i != count_; ++i)
			{
				XmlNodeList link_nodes = trigger_nodes[i - 1].SelectNodes(
					"array[@name=\"outcomingLinks\"]/set");
				foreach (XmlNode link_node in link_nodes)
				{
					try
					{
						// get name
						string ref_name = link_node.SelectSingleNode(
							"string[@name=\"triggerName\"]"
							).InnerText.Trim();
						if (!names.Contains(ref_name))
							continue;
						// get target
						int target = (int)names[ref_name];
						// get group
						string ref_color = link_node.SelectSingleNode(
							"value[@name=\"color\"]"
							).InnerText.Trim();
						int    group     = Array.IndexOf(Link.color_strings_, ref_color);
						if (group < 0)
							throw new Exception("unknown link colour");
						// create link
						string ref_type = link_node.SelectSingleNode(
							"value[@name=\"type\"]"
							).InnerText.Trim();
						bool   is_thin  = ref_type == "THIN";
						if (i != target) // consistency check
							AddLink(i, target, group, is_thin);
					}
					catch (Exception e)
					{
						string msg = e.ToString();
						if (
							DialogResult.OK != MessageBox.Show(
								string.Format(
									"Invalid message link in \"{0}\".\n\n{1}",
									descriptions_[i].name_,
									e),
								"Warning",
								MessageBoxButtons.OKCancel,
								MessageBoxIcon.Warning)
							)
							return false;
					}
				}
			}
			return true;
		}

		/// <summary>
		/// Output to XML.
		/// </summary>
		public void Serialize(ScriptXmlWriter w)
		{
			// start script
			w.WriteStartElement("script");
			w.WriteStartNamedElement("set", "TriggerChain");
			// trigger chain name
			w.WriteElement("string", "name", "");
			// root node
			w.WriteStartNamedElement("set", "[root]");
			w.WriteAttributeString("usage", "private");
			w.WriteStartNamedElement("set", "position");
			w.WriteElement("float", "x", positions_[0].X.ToString());
			w.WriteElement("float", "y", positions_[0].Y.ToString());
			w.WriteEndElement();
			w.WriteEndElement();
			// triggers
			w.WriteStartNamedElement("array", "triggers");
			for (int i = 1; i != count_; ++i)
			{
				ArrayList links = new ArrayList(); // of Link
				foreach(Link link in adjacency_list_)
					if (link.tail_ == i)
						links.Add(link);
				TriggerDescription.State state;
				if (adjacency_matrix_[0, i])
					state = TriggerDescription.State.Checking;
				else
					state = TriggerDescription.State.Sleeping;
				descriptions_[i].Serialize(w, links, descriptions_, state, positions_[i]);
			}
			w.WriteEndElement();
			// layout info
			w.WriteElement("int", "left_",   "0");
			w.WriteElement("int", "top_",    "0");
			w.WriteElement("int", "right_",  "0");
			w.WriteElement("int", "bottom_", "0");
			w.WriteRect("boundingRect", Rectangle.Empty);
			w.WriteRect("viewRect",     Rectangle.Empty);
			// end script
			w.WriteEndElement();
			w.WriteEndElement();
		}

		public void FlipLink(int head, int tail)
		{
			adjacency_matrix_.Flip(tail, head);
			Link link = adjacency_list_.Remove(tail, head);
			adjacency_list_.Add(link);
		}

		#endregion

		#region groups

		public Group GroupTriggers(int[] trigger_indices)
		{
			// create a new group
			Group new_group = new Group();
			groups_.Add(new_group);
			// set new groups
			foreach (int i in trigger_indices)
				descriptions_[i].group_ = new_group;
			// this might have made some groups empty
			// find all nonempty groups
			ArrayList non_empty = new ArrayList(); // of Group
			for (int i = 0; i != count_; ++i)
				if (!non_empty.Contains(descriptions_[i].group_))
					non_empty.Add(descriptions_[i].group_);
			// throw out the empty groups
			groups_ = non_empty;
			return new_group;
		}

		public void Ungroup(Group group)
		{
			for (int i = 0; i != count_; ++i)
				if (descriptions_[i].group_ == group)
					descriptions_[i].group_ = null;
		}

		public void Ungroup(Group[] groups)
		{
			// create a hash table of the groups
			Hashtable group_hash = new Hashtable();
			foreach (Group group in groups)
				group_hash.Add(group, null);
			// ungroup
			for (int i = 0; i != count_; ++i)
				if (group_hash.Contains(descriptions_[i].group_))
					descriptions_[i].group_ = null;
		}

		#endregion

		#region ghosts

		public void AddMarkerGhost(TriggerDescription description, PointF position)
		{
			g_marker_descriptions_.Add(description);
			g_marker_positions_.Add(position);
		}

		public void SetMarkerGhost(int index, TriggerDescription description, PointF position)
		{
			g_marker_descriptions_[index] = description;
			g_marker_positions_   [index] = position;
		}

		public TriggerDescription GetMarkerGhostDescription(int index)
		{
			return (TriggerDescription)g_marker_descriptions_[index];
		}

		public PointF GetMarkerGhostPosition(int index)
		{
			return (PointF)g_marker_positions_[index];
		}

		public void RemoveMarkerGhosts()
		{
			g_marker_descriptions_.Clear();
			g_marker_positions_.Clear();
		}

		public void AddLinkGhost(PointF tail, PointF head)
		{
			g_links_.Add(new GhostLink(tail, head));
		}

		public void AddLinkGhost(GhostLink link)
		{
			g_links_.Add(link);
		}

		public GhostLink GetLinkGhost(int index)
		{
			return (GhostLink)g_links_[index];
		}

		public void SetLinkGhost(int index, PointF tail, PointF head)
		{
			g_links_[index] = new GhostLink(tail, head);
		}

		public void SetLinkGhost(int index, GhostLink link)
		{
			g_links_[index] = link;
		}

		public void RemoveLinkGhosts()
		{
			g_links_.Clear();
		}

		#endregion

		//---------------
		// implementation
		//---------------

		#region

		private void SnapPoints(ref PointF pnt1, ref PointF pnt2, float marker_width, float marker_height)
		{
			// initialize the rectangles
			RectangleF rect1 = new RectangleF(0, 0, marker_width, marker_height);
			RectangleF rect2 = rect1;
			rect2.Offset(pnt2.X - pnt1.X, pnt2.Y - pnt1.Y);
			// snap logic
			if (rect1.Bottom < rect2.Top)
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += marker_width  / 2;
					pnt1.Y += marker_height / 2;
					pnt2.X -= marker_width  / 2;
					pnt2.Y -= marker_height / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= marker_width  / 2;
					pnt1.Y += marker_height / 2;
					pnt2.X += marker_width  / 2;
					pnt2.Y -= marker_height / 2;
				}
				else
				{
					pnt1.Y += marker_height / 2;
					pnt2.Y -= marker_height / 2;
				}
			}
			else if (rect1.Top > rect2.Bottom)
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += marker_width  / 2;
					pnt1.Y -= marker_height / 2;
					pnt2.X -= marker_width  / 2;
					pnt2.Y += marker_height / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= marker_width  / 2;
					pnt1.Y -= marker_height / 2;
					pnt2.X += marker_width  / 2;
					pnt2.Y += marker_height / 2;
				}
				else
				{
					pnt1.Y -= marker_height / 2;
					pnt2.Y += marker_height / 2;
				}
			}
			else
			{
				if (rect1.Right < rect2.Left)
				{
					pnt1.X += marker_width / 2;
					pnt2.X -= marker_width / 2;
				}
				else if (rect1.Left > rect2.Right)
				{
					pnt1.X -= marker_width / 2;
					pnt2.X += marker_width / 2;
				}
			}
		}

		#region entry creation

		private void CreateDescriptionEntry()
		{
			TriggerDescription[] new_descriptions = new TriggerDescription[count_ + 1];
			descriptions_.CopyTo(new_descriptions, 0);
			descriptions_ = new_descriptions;
		}

		private void CreateHistoryEntry()
		{
			PositionHistory[] new_history = new PositionHistory[count_ + 1];
			history_.CopyTo(new_history, 0);
			history_ = new_history;
		}

		private void CreatePositionEntry()
		{
			PointF[] new_positions = new PointF[count_ + 1];
			positions_.CopyTo(new_positions, 0);
			positions_ = new_positions;
		}

		private void CreateVelocityEntry()
		{
			PointF[] new_velocities = new PointF[count_ + 1];
			velocities_.CopyTo(new_velocities, 0);
			velocities_ = new_velocities;
		}

		#endregion

		#region entry deletion

		private void DeleteDescriptionEntry(int index)
		{
			TriggerDescription[] new_descriptions = new TriggerDescription[count_ - 1];
			Array.Copy(descriptions_, 0, new_descriptions, 0, index);
			Array.Copy(descriptions_, index  + 1, new_descriptions, index, count_ - index - 1);
			descriptions_ = new_descriptions;
		}

		private void DeleteHistoryEntry(int index)
		{
			PositionHistory[] new_history = new PositionHistory[count_ - 1];
			Array.Copy(history_, 0, new_history, 0, index);
			Array.Copy(history_, index  + 1, new_history, index, count_ - index - 1);
			history_ = new_history;
		}

		private void DeletePositionEntry(int index)
		{
			PointF[] new_positions = new PointF[count_ - 1];
			Array.Copy(positions_, 0, new_positions, 0, index);
			Array.Copy(positions_, index  + 1, new_positions, index, count_ - index - 1);
			positions_ = new_positions;
		}

		private void DeleteVelocityEntry(int index)
		{
			PointF[] new_velocities = new PointF[count_ - 1];
			Array.Copy(velocities_, 0, new_velocities, 0, index);
			Array.Copy(velocities_, index  + 1, new_velocities, index, count_ - index - 1);
			velocities_ = new_velocities;
		}

		#endregion

		#region entry duplication

		private void DuplicateListEntry(int index)
		{
			// count links
			int link_count = 0;
			for (int i = 0; i != adjacency_list_.Count; ++i)
				if (adjacency_list_[i].head_ == index || adjacency_list_[i].tail_ == index)
					++link_count;
			if (0 == link_count)
				return;
			// copy links
			Link[] links = new Link[link_count];
			int iter = 0;
			for (int i = 0; i != adjacency_list_.Count; ++i)
			{
				if (adjacency_list_[i].head_ == index)
				{
					links[iter] = adjacency_list_[i];
					links[iter].head_ = count_;
					++iter;
				}
				else if (adjacency_list_[i].tail_ == index)
				{
					links[iter] = adjacency_list_[i];
					links[iter].tail_ = count_;
					++iter;
				}
			}
			// insert new links
			for (int i = 0; i != links.Length; ++i)
				adjacency_list_.Add(links[i]);
		}

		private void DuplicateDescriptionEntry(int index)
		{
			CreateDescriptionEntry();
			descriptions_[count_] = descriptions_[index];
		}

		private void DuplicateHistoryEntry(int index)
		{
			CreateHistoryEntry();
			history_[count_] = history_[index];
		}

		private void DuplicatePositionEntry(int index)
		{
			CreatePositionEntry();
			positions_[count_] = positions_[index];
		}

		private void DuplicateVelocityEntrty(int index)
		{
			CreateVelocityEntry();
			velocities_[count_] = velocities_[index];
		}

		#endregion

		#endregion

		//-----
		// data
		//-----

		#region

		// state
		public AdjacencyMatrix      adjacency_matrix_;
		public AdjacencyList        adjacency_list_;
		public int                  count_;
		public TriggerDescription[] descriptions_;
		public ArrayList            groups_; // of Group
		public PositionHistory[]    history_;
		public PointF[]             positions_;
		public PointF[]             velocities_;

		// interpolation constants
		private float ratio_;
		private float r0_;
		private float r1_;
		private float r2_;

		// ghosts
		public ArrayList g_marker_descriptions_;
		public ArrayList g_marker_positions_;
		public ArrayList g_links_;

		#endregion
	}
}
