import React,{Component} from 'react';
import './Tabs.css';

class Tabs extends Component {
    constructor(props){
        super(props);
        this.state = {
            tabActive: 0,
            tabs:[]
        }
    }

    setActive(index){
        this.setState({
            tabActive:index
        })
    }

    render(){

        return(
            <nav className='tab-navigation'>
                <div className='tabs'>
                    {this.props.children.map((child,i) => 
                        <div key={i} onClick={()=>this.setActive(i)} 
                        className={(this.state.tabActive === i)?'active':'inactive'}>
                            {child.title}
                        </div>
                    )}
                </div>
                <div className="panel">
                    {this.props.children[this.state.tabActive].content}
                </div>
            </nav>
        )
    }
}

export default Tabs;