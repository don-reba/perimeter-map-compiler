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

			public void Serialize(XmlNode node, out Point point, out State state)
			{
				// read coordinates
				point = new Point(
					int.Parse(node.SelectSingleNode(
						"set[@name=\"cellIndex\"]/int[@name=\"x\"]").InnerText),
					int.Parse(node.SelectSingleNode(
						"set[@name=\"cellIndex\"]/int[@name=\"y\"]").InnerText));
				// read name
				name_ = node.SelectSingleNode(
					"string[@name=\"name\"]").InnerText;
				// read state
				XmlNode state_node = node.SelectSingleNode(
					"value[@name=\"state_\"]");
				state = State.Sleeping;
				switch (state_node.InnerText)
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
			}

			public void Serialize(ScriptXmlWriter w, IEnumerable link_names, State state)
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
				foreach (string link_name in link_names)
				{
					w.WriteStartElement("set");
					w.WriteElement("string", "triggerName",    link_name);
					w.WriteElement("value",  "color",          "STRATEGY_RED");
					w.WriteElement("value",  "type",           "THIN");
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
				// end trigger
				w.WriteEndElement();
			}

			public Color Fill
			{
				get
				{
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

		#endregion

		//----------
		// interface
		//----------

		#region interface

		public TriggerContainer()
		{
			g_marker_descriptions_ = new ArrayList();
			g_marker_positions_    = new ArrayList();
			g_links_               = new ArrayList();
		}

		public void Collapse(int index)
		{
			adjacency_.Collapse   (index);
			DeleteDescriptionEntry(index);
			DeleteHistoryEntry    (index);
			DeletePositionEntry   (index);
			DeleteVelocityEntry   (index);
			--count_;
		}

		public void AddNew()
		{
			adjacency_.Grow(1);
			CreateDescriptionEntry();
			CreateHistoryEntry();
			CreatePositionEntry();
			CreateVelocityEntry();
			++count_;
		}

		public void Delete(int index)
		{
			adjacency_.Delete     (index);
			DeleteDescriptionEntry(index);
			DeleteHistoryEntry    (index);
			DeletePositionEntry   (index);
			DeleteVelocityEntry   (index);
			--count_;
		}

		public void Duplicate(int index)
		{
			adjacency_.Duplicate     (index);
			DuplicateDescriptionEntry(index);
			DuplicateHistoryEntry    (index);
			DuplicatePositionEntry   (index);
			DuplicateVelocityEntrty  (index);
			++count_;
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
			adjacency_[tail, head] = false;
//			bool connected;
//			connected = false;
//			// switch head and tail, so that tail is greater than head
//			if (head > tail)
//			{
//				int temp = tail;
//				tail     = head;
//				head     = temp;
//			}
//			// remove the tail if it becomes disconnected
//			for (int i = 0; i != count_; ++i)
//				if (adjacency_[tail, i])
//				{
//					connected = true;
//					break;
//				}
//			if (!connected)
//				for (int i = 0; i != count_; ++i)
//					if (adjacency_[i, tail])
//					{
//						connected = true;
//						break;
//					}
//			if (!connected)
//				Delete(tail);
//			// remove the head if it becomes disconnected
//			connected = false;
//			for (int i = 0; i != count_; ++i)
//				if (adjacency_[head, i])
//				{
//					connected = true;
//					break;
//				}
//			if (!connected)
//				for (int i = 0; i != count_; ++i)
//					if (adjacency_[i, head])
//					{
//						connected = true;
//						break;
//					}
//			if (!connected)
//				Delete(head);
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

		public bool Serialize(XmlNode node)
		{
			// extract data
			XmlNodeList trigger_nodes = node.SelectNodes(
				"//script"                     +
				"/set[@name=\"TriggerChain\"]" +
				"/array[@name=\"triggers\"]"   +
				"/set");
			count_ = trigger_nodes.Count + 1; // + the root trigger
			// initialize state variables
			adjacency_    = new AdjacencyMatrix(count_);
			descriptions_ = new TriggerDescription[count_];
			history_      = new PositionHistory[count_];
			positions_    = new PointF[count_];
			velocities_   = new PointF[count_];
			// serialize triggers and read in names
			Hashtable names = new Hashtable(count_);
			descriptions_[0].name_       = "[root]";
			descriptions_[0].is_virtual_ = true;
			for (int i = 1; i != count_; ++i)
			{
				Point point;
				TriggerDescription.State state;
				descriptions_[i].Serialize(trigger_nodes[i - 1], out point, out state);
				if (state == TriggerDescription.State.Checking)
					adjacency_[0, i] = true;
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
						string ref_name = link_node.SelectSingleNode("string[@name=\"triggerName\"]").InnerText;
						if (!names.Contains(ref_name))
							continue;
						int target = (int)names[ref_name];
						if (i != target) // consistency check
							adjacency_[i, target] = true;
					}
					catch (Exception e)
					{
						string msg = e.ToString();
						if (
							DialogResult.OK != MessageBox.Show(
								"Invalid message link in \""
								+ descriptions_[i].name_
								+ "\".",
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

		public void Serialize(ScriptXmlWriter w)
		{
			// start script
			w.WriteStartElement("script");
			w.WriteStartNamedElement("set", "TriggerChain");
			// trigger chain name
			w.WriteElement("string", "name", "");
			// triggers
			w.WriteStartNamedElement("array", "triggers");
			for (int i = 1; i != count_; ++i)
			{
				ArrayList link_names = new ArrayList();
				foreach (int j in adjacency_.GetList(i))
					link_names.Add(descriptions_[j].name_);
				TriggerDescription.State state;
				if (adjacency_[0, i])
					state = TriggerDescription.State.Checking;
				else
					state = TriggerDescription.State.Sleeping;
				descriptions_[i].Serialize(w, link_names, state);
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

		public void AddLinkGhost(PointF head, PointF tail)
		{
			Link link;
			link.head_      = head;
			link.tail_      = tail;
			link.snap_head_ = true;
			link.snap_tail_ = true;
			g_links_.Add(link);
		}

		public void AddLinkGhost(Link link)
		{
			g_links_.Add(link);
		}

		public Link GetLinkGhost(int index)
		{
			return (Link)g_links_[index];
		}

		public void SetLinkGhost(int index, PointF head, PointF tail)
		{
			Link link;
			link.head_      = head;
			link.tail_      = tail;
			link.snap_head_ = true;
			link.snap_tail_ = true;
			g_links_[index] = link;
		}

		public void SetLinkGhost(int index, Link link)
		{
			g_links_[index] = link;
		}

		public void RemoveLinkGhosts()
		{
			g_links_.Clear();
		}

		#endregion
		
		#endregion

		//---------------
		// implementation
		//---------------

		#region

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
		public AdjacencyMatrix      adjacency_;
		public int                  count_;
		public TriggerDescription[] descriptions_;
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
