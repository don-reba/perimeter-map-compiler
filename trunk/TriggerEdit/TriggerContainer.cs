using System;
using System.Collections;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Xml;
using TriggerEdit.Definitions;

namespace TriggerEdit
{
	public class TriggerContainer
	{
		#region nested types

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
				Bright = 1,
				Dim    = 2,
				Ghost  = 4,
				Selected = 8
			}

			#endregion

			#region interface

			public void Serialize(XmlNode node, out Point point)
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
				switch (state_node.InnerText)
				{
					case "CHECKING": state_ = State.Checking; break;
					case "SLEEPING": state_ = State.Sleeping; break;
					case "DONE":     state_ = State.Done;     break;
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

			public XmlNode Serialize()
			{
				return null;
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
						return Color.Yellow;
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
			public State     state_;
			public Status    status_;

			#endregion
		}

		public struct PositionHistory
		{
			public void Add(PointF point)
			{
				point1_ = point2_;
				point2_ = point3_;
				point3_ = point;
			}
			// least recent
			public PointF point1_;
			public PointF point2_;
			public PointF point3_;
			// most recent
		}

		#endregion

		#region interface

		public TriggerContainer()
		{
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

		public void Serialize(XmlNode node)
		{
			// extract data
			XmlNodeList trigger_nodes = node.SelectNodes(
				"//script"                     +
				"/set[@name=\"TriggerChain\"]" +
				"/array[@name=\"triggers\"]"   +
				"/set");
			count_ = trigger_nodes.Count;
			// initialize state variables
			adjacency_    = new AdjacencyMatrix(count_);
			descriptions_ = new TriggerDescription[count_];
			history_      = new PositionHistory[count_];
			positions_    = new PointF[count_];
			velocities_   = new PointF[count_];
			// serialize triggers and read in names
			Hashtable names = new Hashtable(count_);;
			for (int i = 0; i != count_; ++i)
			{
				Point point;
				descriptions_[i].Serialize(trigger_nodes[i], out point);
				names.Add(descriptions_[i].name_, i);
			}
			// add links
			for (int i = 0; i != count_; ++i)
			{
				XmlNodeList link_nodes = trigger_nodes[i].SelectNodes(
					"array[@name=\"outcomingLinks\"]/set");
				foreach (XmlNode link_node in link_nodes)
				{
						int target = (int)names[
							link_node.SelectSingleNode("string[@name=\"triggerName\"]").InnerText];
					adjacency_[i, target] = true;
				}
			}
		}

		public XmlNode Serialize()
		{
			return null;
		}

		#endregion

		#region data

		public AdjacencyMatrix      adjacency_;
		public int                  count_;
		public TriggerDescription[] descriptions_;
		public PositionHistory[]    history_;
		public PointF[]             positions_;
		public PointF[]             velocities_;

		#endregion
	}
}
