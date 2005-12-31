using System;
using System.Collections;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Windows.Forms;
using TriggerEdit.Definitions;

namespace TriggerEdit
{
	public class ConditionDisplay : System.Windows.Forms.Panel
	{
		#region nested data

		private class LayoutElement
		{
			public LayoutElement(LayoutElement parent, Condition link)
			{
				parent_ = parent;
				link_   = link;
			}
			public LayoutElement(LayoutElement parent, Condition link, uint size)
				:this(parent, link)
			{
				size_ = size;
			}
			public string Name
			{
				get
				{
					if (null == link_)
						return "[new]";
					string name = link_.Name;
					switch (name)
					{
						case "Switcher":
							return ((ConditionSwitcher)link_).type.ToString();
						default:
							return name;
					}
				}
			}
			public Rectangle     marker_;
			public LayoutElement parent_;
			public ArrayList     children_; // of LayoutElement
			public Condition     link_;
			public uint          size_;
		}

		#endregion

		#region interface

		public ConditionDisplay()
		{
			spacing_ = new Size(32, 16);
		}

		public Condition Condition
		{
			set
			{
				condition_ = value;
				if (null == condition_)
					tree_ = new LayoutElement(null, null, 1);
				else
				{
					tree_ = new LayoutElement(null, condition_);
					SizeElement(ref tree_);
				}
				Refresh();
			}
		}

		#endregion

		#region events

		public class ConditionEventArgs : EventArgs
		{
			public ConditionEventArgs(Condition condition, Condition parent)
			{
				condition_ = condition;
				parent_    = parent;
			}
			public readonly Condition condition_;
			public readonly Condition parent_;
		}

		public delegate void SelectConditionEventHandler(object sender, ConditionEventArgs e);

		public event SelectConditionEventHandler SelectCondition;

		protected virtual void OnSelectCondition(ConditionEventArgs e)
		{
			if (null != SelectCondition)
				SelectCondition(this, e);
		}

		public delegate void NewConditionEventHandler(object sender, ConditionEventArgs e);

		public event NewConditionEventHandler NewCondition;

		protected virtual void OnNewCondition(ConditionEventArgs e)
		{
			if (null != NewCondition)
				NewCondition(this, e);
		}

		#endregion

		#region overrides

		protected override void OnMouseDown(MouseEventArgs e)
		{
			Point click_coords = new Point(e.X, e.Y);
			selection_ = PointToElement(click_coords, tree_);
			Refresh();
			if (null != selection_)
				if (null == selection_.link_)
					OnNewCondition(new ConditionEventArgs(
						selection_.link_,
						selection_.parent_.link_));
				else
					OnSelectCondition(new ConditionEventArgs(
						selection_.link_,
						(null == selection_.parent_) ? null : selection_.parent_.link_));
		}


		protected override void OnPaint(PaintEventArgs e)
		{
			e.Graphics.Clear(Color.Black);
			if (null == condition_)
				return;
			PlaceElement(e.Graphics, ref tree_, new Point(8, ClientRectangle.Height / 2));
			DrawElement(e.Graphics, tree_);
		}

		protected override void OnPaintBackground(PaintEventArgs pevent)
		{
			if (DesignMode)
				base.OnPaintBackground(pevent);
		}

		protected override void OnResize(EventArgs e)
		{
			base.OnResize (e);
			Invalidate();
		}

		#endregion

		#region internal implementation

		private void DrawElement(Graphics g, LayoutElement e)
		{
			g.SmoothingMode = SmoothingMode.None;
			// draw marker
			g.FillRectangle((e == selection_) ? Brushes.Yellow : Brushes.Orange, e.marker_);
			// draw name
			g.DrawString(e.Name, Font, Brushes.Black, e.marker_.Left + 2, e.marker_.Top + 1);
			// return if leaf
			if (null == e.children_)
				return;
			// draw leaves
			foreach (LayoutElement child in e.children_)
			{
				g.SmoothingMode = SmoothingMode.AntiAlias;
				g.DrawLine(
					Pens.Orange,
					e.marker_.Right,
					e.marker_.Top + e.marker_.Height / 2,
					child.marker_.Left,
					child.marker_.Top + child.marker_.Height / 2);
				DrawElement(g, child);
			}
		}

		private void PlaceElement(Graphics g, ref LayoutElement e, Point position)
		{
			SizeF size = g.MeasureString(e.Name, Font);
			e.marker_ = new Rectangle(
				position.X,
				position.Y - (int)(size.Height / 2.0f),
				(int)size.Width,
				(int)size.Height);
			e.marker_.Inflate(2, 1);
			if (null == e.children_)
				return;
			Point child_position = new Point(
				e.marker_.Right + spacing_.Width,
				position.Y - (int)e.size_ * (16 + spacing_.Height) / 2);
			for (int i = 0; i != e.children_.Count; ++i)
			{
				LayoutElement child = (LayoutElement)e.children_[i];
				int height = (16 + spacing_.Height) * (int)child.size_;
				child_position.Y += height / 2;
				PlaceElement(g, ref child, child_position);
				child_position.Y += height - height / 2;
			}
		}
		private LayoutElement PointToElement(Point point, LayoutElement e)
		{
			if (
				point.X >= e.marker_.Left  &&
				point.X <= e.marker_.Right &&
				point.Y >= e.marker_.Top   &&
				point.Y <= e.marker_.Bottom)
				return e;
			if (null == e.children_)
				return null;
			foreach (LayoutElement child in e.children_)
			{
				LayoutElement found = PointToElement(point, child);
				if (null != found)
					return found;
			}
			return null;
		}

		private void SizeElement(ref LayoutElement e)
		{
			if (null == e.link_.preconditions)
			{
				e.size_ = 1;
				return;
			}
			e.children_ = new ArrayList(e.link_.preconditions.Count + 1);
			foreach (Condition condition in e.link_.preconditions)
			{
				LayoutElement child = new LayoutElement(e, condition);
				SizeElement(ref child);
				e.size_ += child.size_;
				e.children_.Add(child);
			}
			++e.size_;
			e.children_.Add(new LayoutElement(e, null, 1));
		}

		#endregion

		#region data

		Condition     condition_;
		LayoutElement selection_;
		Size          spacing_;
		LayoutElement tree_;

		#endregion
	}
}
