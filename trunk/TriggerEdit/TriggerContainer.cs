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
				None     = 0x00,
				Bright   = 0x01,
				Dim      = 0x02,
				Ghost    = 0x04,
				Selected = 0x08,
				All      = 0xFF
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
			public float Norm(float x, float y)
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

		#region interface

		public TriggerContainer()
		{
			ghost_index_ = -1;
		}

		public void Delete(int index)
		{
			// adjacency
			string before = "";
			for (int row = 0; row != adjacency_.Count; ++row)
			{
				for (int col = 0; col != adjacency_.Count; ++col)
					before += adjacency_[row, col] ? '-' : '-';
				before += '\n';
			}
			adjacency_.Delete(index);
			string after = "";
			for (int row = 0; row != adjacency_.Count; ++row)
			{
				for (int col = 0; col != adjacency_.Count; ++col)
					after += adjacency_[row, col] ? '-' : '-';
				after += '\n';
			}
			// descriptions
			TriggerDescription[] new_descriptions = new TriggerDescription[count_ - 1];
			Array.Copy(descriptions_, 0, new_descriptions, 0, index);
			Array.Copy(descriptions_, index  + 1, new_descriptions, index, count_ - index - 1);
			descriptions_ = new_descriptions;
			// history
			PositionHistory[] new_history = new PositionHistory[count_ - 1];
			Array.Copy(history_, 0, new_history, 0, index);
			Array.Copy(history_, index  + 1, new_history, index, count_ - index - 1);
			history_ = new_history;
			// positions
			PointF[] new_positions = new PointF[count_ - 1];
			Array.Copy(positions_, 0, new_positions, 0, index);
			Array.Copy(positions_, index  + 1, new_positions, index, count_ - index - 1);
			positions_ = new_positions;
			// velocities
			PointF[] new_velocities = new PointF[count_ - 1];
			Array.Copy(velocities_, 0, new_velocities, 0, index);
			Array.Copy(velocities_, index  + 1, new_velocities, index, count_ - index - 1);
			velocities_ = new_velocities;
			// count
			--count_;
		}

		public void Duplicate(int index)
		{
			// adjacency
			adjacency_.Grow(1);
			for (int i = 0; i != count_; ++i)
				adjacency_[count_, i] = adjacency_[index, i];
			for (int i = 0; i != count_; ++i)
				adjacency_[i, count_] = adjacency_[i, index];
			// descriptions
			TriggerDescription[] new_descriptions = new TriggerDescription[count_ + 1];
			descriptions_.CopyTo(new_descriptions, 0);
			new_descriptions[count_] = descriptions_[index];
			descriptions_ = new_descriptions;
			// history
			PositionHistory[] new_history = new PositionHistory[count_ + 1];
			history_.CopyTo(new_history, 0);
			new_history[count_] = history_[index];
			history_ = new_history;
			// positions
			PointF[] new_positions = new PointF[count_ + 1];
			positions_.CopyTo(new_positions, 0);
			new_positions[count_] = positions_[index];
			positions_ = new_positions;
			// velocities
			PointF[] new_velocities = new PointF[count_ + 1];
			velocities_.CopyTo(new_velocities, 0);
			new_velocities[count_] = velocities_[index];
			velocities_ = new_velocities;
			// count
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

		public void GetInterpolatedPosition(int index, ref PointF point)
		{
			PositionHistory src = history_[index];
			point.X = src.point1_.X * r0_ + src.point2_.X * r1_ + src.point3_.X * r2_;
			point.Y = src.point1_.Y * r0_ + src.point2_.Y * r1_ + src.point3_.Y * r2_;
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
			bool connected;
			connected = false;
			// switch head and tail, so that tail is greater than head
			if (head > tail)
			{
				int temp = tail;
				tail     = head;
				head     = temp;
			}
			// remove the tail if it becomes disconnected
			for (int i = 0; i != count_; ++i)
				if (adjacency_[tail, i])
				{
					connected = true;
					break;
				}
			if (!connected)
				for (int i = 0; i != count_; ++i)
					if (adjacency_[i, tail])
					{
						connected = true;
						break;
					}
			if (!connected)
				Delete(tail);
			// remove the head if it becomes disconnected
			connected = false;
			for (int i = 0; i != count_; ++i)
				if (adjacency_[head, i])
				{
					connected = true;
					break;
				}
			if (!connected)
				for (int i = 0; i != count_; ++i)
					if (adjacency_[i, head])
					{
						connected = true;
						break;
					}
			if (!connected)
				Delete(head);
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
			Hashtable names = new Hashtable(count_);
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
					if (i != target) // consistency check
						adjacency_[i, target] = true;
				}
			}
			// other initialization
			ghost_index_ = -1;
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
		public int                  ghost_index_;
		public PointF               ghost_position_;
		public PositionHistory[]    history_;
		public PointF[]             positions_;
		private float               ratio_;
		private float               r0_;
		private float               r1_;
		private float               r2_;
		public PointF[]             velocities_;

		#endregion
	}
}
