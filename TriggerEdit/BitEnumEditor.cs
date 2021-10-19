using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace TriggerEdit
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class BitEnumEditorForm : System.Windows.Forms.Form
	{
		#region interface

		public BitEnumEditorForm()
		{
			InitializeComponent();
			ResizePanels();
		}

		public BitEnum BitEnum
		{
			set
			{
				bit_enum_ = value;
				// populate the "available values" listbox
				names_ = Enum.GetNames(bit_enum_.GetEnumType());
				Array.Sort(names_);
				available_lst_.Items.AddRange(names_);
				// populate the "selected values" listbox
				ArrayList selected = bit_enum_.GetList();
				foreach (object field in selected)
				{
					int index = Array.BinarySearch(names_, field.ToString());
					if (index >= 0)
						available_lst_.SetSelected(index, true);
				}
			}
		}

		#endregion

		#region implementation

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
				if(components != null)
					components.Dispose();
			base.Dispose(disposing);
		}

		private void available_lst__SelectedIndexChanged(object sender, System.EventArgs e)
		{
			selected_lst_.Items.Clear();
			foreach (int index in available_lst_.SelectedIndices)
				selected_lst_.Items.Add(names_[index]);
		}

		private void ok_btn__Click(object sender, System.EventArgs e)
		{
			bit_enum_.SetNone();
			foreach (int index in available_lst_.SelectedIndices)
				bit_enum_[Enum.Parse(bit_enum_.GetEnumType(), names_[index])] = true;
		}

		private void BitEnumEditorForm_Resize(object sender, System.EventArgs e)
		{
			ResizePanels();
		}

		private void ResizePanels()
		{
			left_pnl_.SetBounds(
				0,
				0,
				body_pnl_.Width  / 2,
				body_pnl_.Height);
			right_pnl_.SetBounds(
				left_pnl_.Width,
				0,
				body_pnl_.Width - left_pnl_.Width,
				body_pnl_.Height);
		}

		#endregion

		#region data

		private BitEnum   bit_enum_;
		private string[]  names_;

		#endregion

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.available_lst_ = new System.Windows.Forms.ListBox();
			this.ok_btn_ = new System.Windows.Forms.Button();
			this.cancel_btn_ = new System.Windows.Forms.Button();
			this.label1 = new System.Windows.Forms.Label();
			this.selected_lst_ = new System.Windows.Forms.ListBox();
			this.label2 = new System.Windows.Forms.Label();
			this.body_pnl_ = new System.Windows.Forms.Panel();
			this.button_pnl_ = new System.Windows.Forms.Panel();
			this.left_pnl_ = new System.Windows.Forms.Panel();
			this.right_pnl_ = new System.Windows.Forms.Panel();
			this.body_pnl_.SuspendLayout();
			this.button_pnl_.SuspendLayout();
			this.left_pnl_.SuspendLayout();
			this.right_pnl_.SuspendLayout();
			this.SuspendLayout();
			// 
			// available_lst_
			// 
			this.available_lst_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.available_lst_.HorizontalScrollbar = true;
			this.available_lst_.IntegralHeight = false;
			this.available_lst_.Location = new System.Drawing.Point(0, 16);
			this.available_lst_.Name = "available_lst_";
			this.available_lst_.SelectionMode = System.Windows.Forms.SelectionMode.MultiSimple;
			this.available_lst_.Size = new System.Drawing.Size(144, 176);
			this.available_lst_.TabIndex = 3;
			this.available_lst_.SelectedIndexChanged += new System.EventHandler(this.available_lst__SelectedIndexChanged);
			// 
			// ok_btn_
			// 
			this.ok_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.ok_btn_.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.ok_btn_.Location = new System.Drawing.Point(152, 16);
			this.ok_btn_.Name = "ok_btn_";
			this.ok_btn_.TabIndex = 5;
			this.ok_btn_.Text = "OK";
			this.ok_btn_.Click += new System.EventHandler(this.ok_btn__Click);
			// 
			// cancel_btn_
			// 
			this.cancel_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.cancel_btn_.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.cancel_btn_.Location = new System.Drawing.Point(240, 16);
			this.cancel_btn_.Name = "cancel_btn_";
			this.cancel_btn_.TabIndex = 6;
			this.cancel_btn_.Text = "Cancel";
			// 
			// label1
			// 
			this.label1.Dock = System.Windows.Forms.DockStyle.Top;
			this.label1.Location = new System.Drawing.Point(0, 0);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(144, 16);
			this.label1.TabIndex = 8;
			this.label1.Text = "Available Values";
			// 
			// selected_lst_
			// 
			this.selected_lst_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.selected_lst_.HorizontalScrollbar = true;
			this.selected_lst_.IntegralHeight = false;
			this.selected_lst_.Location = new System.Drawing.Point(8, 16);
			this.selected_lst_.Name = "selected_lst_";
			this.selected_lst_.SelectionMode = System.Windows.Forms.SelectionMode.None;
			this.selected_lst_.Size = new System.Drawing.Size(144, 176);
			this.selected_lst_.TabIndex = 1;
			// 
			// label2
			// 
			this.label2.Dock = System.Windows.Forms.DockStyle.Top;
			this.label2.Location = new System.Drawing.Point(8, 0);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(144, 16);
			this.label2.TabIndex = 9;
			this.label2.Text = "Selected Values";
			// 
			// body_pnl_
			// 
			this.body_pnl_.Controls.Add(this.right_pnl_);
			this.body_pnl_.Controls.Add(this.left_pnl_);
			this.body_pnl_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.body_pnl_.Location = new System.Drawing.Point(16, 16);
			this.body_pnl_.Name = "body_pnl_";
			this.body_pnl_.Size = new System.Drawing.Size(320, 205);
			this.body_pnl_.TabIndex = 10;
			// 
			// button_pnl_
			// 
			this.button_pnl_.Controls.Add(this.ok_btn_);
			this.button_pnl_.Controls.Add(this.cancel_btn_);
			this.button_pnl_.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.button_pnl_.Location = new System.Drawing.Point(16, 221);
			this.button_pnl_.Name = "button_pnl_";
			this.button_pnl_.Size = new System.Drawing.Size(320, 40);
			this.button_pnl_.TabIndex = 11;
			// 
			// left_pnl_
			// 
			this.left_pnl_.Controls.Add(this.available_lst_);
			this.left_pnl_.Controls.Add(this.label1);
			this.left_pnl_.DockPadding.Right = 8;
			this.left_pnl_.Location = new System.Drawing.Point(8, 8);
			this.left_pnl_.Name = "left_pnl_";
			this.left_pnl_.Size = new System.Drawing.Size(152, 192);
			this.left_pnl_.TabIndex = 10;
			// 
			// right_pnl_
			// 
			this.right_pnl_.Controls.Add(this.selected_lst_);
			this.right_pnl_.Controls.Add(this.label2);
			this.right_pnl_.DockPadding.Left = 8;
			this.right_pnl_.Location = new System.Drawing.Point(160, 8);
			this.right_pnl_.Name = "right_pnl_";
			this.right_pnl_.Size = new System.Drawing.Size(152, 192);
			this.right_pnl_.TabIndex = 11;
			// 
			// BitEnumEditorForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(352, 277);
			this.Controls.Add(this.body_pnl_);
			this.Controls.Add(this.button_pnl_);
			this.DockPadding.All = 16;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.MinimumSize = new System.Drawing.Size(300, 200);
			this.Name = "BitEnumEditorForm";
			this.ShowInTaskbar = false;
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
			this.Text = "Flag Editor";
			this.Resize += new System.EventHandler(this.BitEnumEditorForm_Resize);
			this.body_pnl_.ResumeLayout(false);
			this.button_pnl_.ResumeLayout(false);
			this.left_pnl_.ResumeLayout(false);
			this.right_pnl_.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Designer data
		
		private System.ComponentModel.Container components = null;
		private System.Windows.Forms.ListBox available_lst_;
		private System.Windows.Forms.Label   label1;
		private System.Windows.Forms.ListBox selected_lst_;
		private System.Windows.Forms.Label   label2;
		private System.Windows.Forms.Button  ok_btn_;
		private System.Windows.Forms.Button  cancel_btn_;
		private System.Windows.Forms.Panel body_pnl_;
		private System.Windows.Forms.Panel button_pnl_;
		private System.Windows.Forms.Panel left_pnl_;
		private System.Windows.Forms.Panel right_pnl_;

		#endregion
	}
}
