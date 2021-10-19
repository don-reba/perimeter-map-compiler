using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace TriggerEdit
{
	/// <summary>
	/// Summary description for GroupCommentEditor.
	/// </summary>
	public class GroupCommentEditor : System.Windows.Forms.Form
	{

		//----------
		// interface
		//----------

		#region

		public GroupCommentEditor()
		{
			InitializeComponent();
		}

		public string Comment
		{
			get { return text_edt_.Text; }
			set { text_edt_.Text = value; }
		}

		#endregion

		//---------------
		// implementation
		//---------------

		#region

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#endregion

		//---------------
		// event handlers
		//---------------

		private void ok_btn__Click(object sender, System.EventArgs e)
		{
			Close();
		}

		//----------
		// generated
		//----------

		#region code

		private void InitializeComponent()
		{
			this.text_edt_ = new System.Windows.Forms.TextBox();
			this.bottom_pnl_ = new System.Windows.Forms.Panel();
			this.ok_btn_ = new System.Windows.Forms.Button();
			this.bottom_pnl_.SuspendLayout();
			this.SuspendLayout();
			// 
			// text_edt_
			// 
			this.text_edt_.AcceptsReturn = true;
			this.text_edt_.Dock = System.Windows.Forms.DockStyle.Fill;
			this.text_edt_.Location = new System.Drawing.Point(16, 16);
			this.text_edt_.Multiline = true;
			this.text_edt_.Name = "text_edt_";
			this.text_edt_.Size = new System.Drawing.Size(260, 86);
			this.text_edt_.TabIndex = 1;
			this.text_edt_.Text = "textBox1";
			// 
			// bottom_pnl_
			// 
			this.bottom_pnl_.Controls.Add(this.ok_btn_);
			this.bottom_pnl_.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.bottom_pnl_.Location = new System.Drawing.Point(16, 102);
			this.bottom_pnl_.Name = "bottom_pnl_";
			this.bottom_pnl_.Size = new System.Drawing.Size(260, 40);
			this.bottom_pnl_.TabIndex = 2;
			// 
			// ok_btn_
			// 
			this.ok_btn_.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.ok_btn_.Location = new System.Drawing.Point(180, 16);
			this.ok_btn_.Name = "ok_btn_";
			this.ok_btn_.TabIndex = 0;
			this.ok_btn_.Text = "OK";
			this.ok_btn_.Click += new System.EventHandler(this.ok_btn__Click);
			// 
			// GroupCommentEditor
			// 
			this.AcceptButton = this.ok_btn_;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(292, 158);
			this.Controls.Add(this.text_edt_);
			this.Controls.Add(this.bottom_pnl_);
			this.DockPadding.All = 16;
			this.Name = "GroupCommentEditor";
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
			this.Text = "Group Comment";
			this.bottom_pnl_.ResumeLayout(false);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.TextBox text_edt_;
		private System.Windows.Forms.Panel bottom_pnl_;
		private System.Windows.Forms.Button ok_btn_;

		#region data

		private System.ComponentModel.Container components = null;

		#endregion
	}
}
